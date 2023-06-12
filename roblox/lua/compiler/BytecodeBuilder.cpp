// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "BytecodeBuilder.h"

#include "StringUtils.h"

#include <algorithm>
#include <string.h>

#include "../../../security/xor.hpp"

namespace Compiler
{

static const uint32_t kMaxConstantCount = 1 << 23;
static const uint32_t kMaxClosureCount = 1 << 15;

static const int kMaxJumpDistance = 1 << 23;

static int log2(int v)
{
    LUAU_ASSERT(v);

    int r = 0;

    while (v >= (2 << r))
        r++;

    return r;
}

static void writeByte(std::string& ss, unsigned char value)
{
    ss.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

static void writeInt(std::string& ss, int value)
{
    ss.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

static void writeDouble(std::string& ss, double value)
{
    ss.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

static void writeVarInt(std::string& ss, unsigned int value)
{
    do
    {
        writeByte(ss, (value & 127) | ((value > 127) << 7));
        value >>= 7;
    } while (value);
}

static int getOpLength(LuauOpcode op)
{
    switch (op)
    {
    case LOP_GETGLOBAL:
    case LOP_SETGLOBAL:
    case LOP_GETIMPORT:
    case LOP_GETTABLEKS:
    case LOP_SETTABLEKS:
    case LOP_NAMECALL:
    case LOP_JUMPIFEQ:
    case LOP_JUMPIFLE:
    case LOP_JUMPIFLT:
    case LOP_JUMPIFNOTEQ:
    case LOP_JUMPIFNOTLE:
    case LOP_JUMPIFNOTLT:
    case LOP_NEWTABLE:
    case LOP_SETLIST:
    case LOP_FORGLOOP:
    case LOP_LOADKX:
    case LOP_JUMPIFEQK:
    case LOP_JUMPIFNOTEQK:
    case LOP_FASTCALL2:
    case LOP_FASTCALL2K:
        return 2;

    default:
        return 1;
    }
}

inline bool isJumpD(LuauOpcode op)
{
    switch (op)
    {
    case LOP_JUMP:
    case LOP_JUMPIF:
    case LOP_JUMPIFNOT:
    case LOP_JUMPIFEQ:
    case LOP_JUMPIFLE:
    case LOP_JUMPIFLT:
    case LOP_JUMPIFNOTEQ:
    case LOP_JUMPIFNOTLE:
    case LOP_JUMPIFNOTLT:
    case LOP_FORNPREP:
    case LOP_FORNLOOP:
    case LOP_FORGLOOP:
    case LOP_FORGPREP_INEXT:
    case LOP_FORGLOOP_INEXT:
    case LOP_FORGPREP_NEXT:
    case LOP_FORGLOOP_NEXT:
    case LOP_JUMPBACK:
    case LOP_JUMPIFEQK:
    case LOP_JUMPIFNOTEQK:
        return true;

    default:
        return false;
    }
}

inline bool isSkipC(LuauOpcode op)
{
    switch (op)
    {
    case LOP_LOADB:
    case LOP_FASTCALL:
    case LOP_FASTCALL1:
    case LOP_FASTCALL2:
    case LOP_FASTCALL2K:
        return true;

    default:
        return false;
    }
}

bool BytecodeBuilder::StringRef::operator==(const StringRef& other) const
{
    return (data && other.data) ? (length == other.length && memcmp(data, other.data, length) == 0) : (data == other.data);
}

bool BytecodeBuilder::TableShape::operator==(const TableShape& other) const
{
    return length == other.length && memcmp(keys, other.keys, length * sizeof(keys[0])) == 0;
}

size_t BytecodeBuilder::StringRefHash::operator()(const StringRef& v) const
{
    return hashRange(v.data, v.length);
}

size_t BytecodeBuilder::ConstantKeyHash::operator()(const ConstantKey& key) const
{
    // finalizer from MurmurHash64B
    const uint32_t m = 0x5bd1e995;

    uint32_t h1 = uint32_t(key.value);
    uint32_t h2 = uint32_t(key.value >> 32) ^ (key.type * m);

    h1 ^= h2 >> 18;
    h1 *= m;
    h2 ^= h1 >> 22;
    h2 *= m;
    h1 ^= h2 >> 17;
    h1 *= m;
    h2 ^= h1 >> 19;
    h2 *= m;

    // ... truncated to 32-bit output (normally hash is equal to (uint64_t(h1) << 32) | h2, but we only really need the lower 32-bit half)
    return size_t(h2);
}

size_t BytecodeBuilder::TableShapeHash::operator()(const TableShape& v) const
{
    // FNV-1a inspired hash (note that we feed integers instead of bytes)
    uint32_t hash = 2166136261;

    for (size_t i = 0; i < v.length; ++i)
    {
        hash ^= v.keys[i];
        hash *= 16777619;
    }

    return hash;
}

BytecodeBuilder::BytecodeBuilder(BytecodeEncoder* encoder)
    : constantMap({Constant::Type_Nil, ~0ull})
    , tableShapeMap(TableShape())
    , stringTable({nullptr, 0})
    , encoder(encoder)
{
    LUAU_ASSERT(stringTable.find(StringRef{"", 0}) == nullptr);
}

uint32_t BytecodeBuilder::beginFunction(uint8_t numparams, bool isvararg)
{
    LUAU_ASSERT(currentFunction == ~0u);

    uint32_t id = uint32_t(functions.size());

    Function func;
    func.numparams = numparams;
    func.isvararg = isvararg;

    functions.push_back(func);

    currentFunction = id;

    hasLongJumps = false;
    debugLine = 0;

    return id;
}

void BytecodeBuilder::endFunction(uint8_t maxstacksize, uint8_t numupvalues)
{
    LUAU_ASSERT(currentFunction != ~0u);

    Function& func = functions[currentFunction];

    func.maxstacksize = maxstacksize;
    func.numupvalues = numupvalues;

#ifdef LUAU_ASSERTENABLED
    validate();
#endif

    // very approximate: 4 bytes per instruction for code, 1 byte for debug line, and 1-2 bytes for aux data like constants
    func.data.reserve(insns.size() * 7);

    writeFunction(func.data, currentFunction);

    currentFunction = ~0u;

    // this call is indirect to make sure we only gain link time dependency on dumpCurrentFunction when needed
    if (dumpFunctionPtr)
        func.dump = (this->*dumpFunctionPtr)();

    insns.clear();
    lines.clear();
    constants.clear();
    protos.clear();
    jumps.clear();
    tableShapes.clear();

    debugLocals.clear();
    debugUpvals.clear();

    constantMap.clear();
    tableShapeMap.clear();
}

void BytecodeBuilder::setMainFunction(uint32_t fid)
{
    mainFunction = fid;
}

int32_t BytecodeBuilder::addConstant(const ConstantKey& key, const Constant& value)
{
    if (int32_t* cache = constantMap.find(key))
        return *cache;

    uint32_t id = uint32_t(constants.size());

    if (id >= kMaxConstantCount)
        return -1;

    constantMap[key] = int32_t(id);
    constants.push_back(value);

    return int32_t(id);
}

unsigned int BytecodeBuilder::addStringTableEntry(StringRef value)
{
    unsigned int& index = stringTable[value];

    // note: bytecode serialization format uses 1-based table indices, 0 is reserved to mean nil
    if (index == 0)
        index = uint32_t(stringTable.size());

    return index;
}

int32_t BytecodeBuilder::addConstantNil()
{
    Constant c = {Constant::Type_Nil};

    ConstantKey k = {Constant::Type_Nil};
    return addConstant(k, c);
}

int32_t BytecodeBuilder::addConstantBoolean(bool value)
{
    Constant c = {Constant::Type_Boolean};
    c.valueBoolean = value;

    ConstantKey k = {Constant::Type_Boolean, value};
    return addConstant(k, c);
}

int32_t BytecodeBuilder::addConstantNumber(double value)
{
    Constant c = {Constant::Type_Number};
    c.valueNumber = value;

    ConstantKey k = {Constant::Type_Number};
    static_assert(sizeof(k.value) == sizeof(value), "Expecting double to be 64-bit");
    memcpy(&k.value, &value, sizeof(value));

    return addConstant(k, c);
}

int32_t BytecodeBuilder::addConstantString(StringRef value)
{
    unsigned int index = addStringTableEntry(value);

    Constant c = {Constant::Type_String};
    c.valueString = index;

    ConstantKey k = {Constant::Type_String, index};

    return addConstant(k, c);
}

int32_t BytecodeBuilder::addImport(uint32_t iid)
{
    Constant c = {Constant::Type_Import};
    c.valueImport = iid;

    ConstantKey k = {Constant::Type_Import, iid};

    return addConstant(k, c);
}

int32_t BytecodeBuilder::addConstantTable(const TableShape& shape)
{
    if (int32_t* cache = tableShapeMap.find(shape))
        return *cache;

    uint32_t id = uint32_t(constants.size());

    if (id >= kMaxConstantCount)
        return -1;

    Constant value = {Constant::Type_Table};
    value.valueTable = uint32_t(tableShapes.size());

    tableShapeMap[shape] = int32_t(id);
    tableShapes.push_back(shape);
    constants.push_back(value);

    return int32_t(id);
}

int32_t BytecodeBuilder::addConstantClosure(uint32_t fid)
{
    Constant c = {Constant::Type_Closure};
    c.valueClosure = fid;

    ConstantKey k = {Constant::Type_Closure, fid};

    return addConstant(k, c);
}

int16_t BytecodeBuilder::addChildFunction(uint32_t fid)
{
    uint32_t id = uint32_t(protos.size());

    if (id >= kMaxClosureCount)
        return -1;

    protos.push_back(fid);

    return int16_t(id);
}

void BytecodeBuilder::emitABC(LuauOpcode op, uint8_t a, uint8_t b, uint8_t c)
{
    uint32_t insn = uint32_t(op) | (a << 8) | (b << 16) | (c << 24);

    insns.push_back(insn);
    lines.push_back(debugLine);
}

void BytecodeBuilder::emitAD(LuauOpcode op, uint8_t a, int16_t d)
{
    uint32_t insn = uint32_t(op) | (a << 8) | (uint16_t(d) << 16);

    insns.push_back(insn);
    lines.push_back(debugLine);
}

void BytecodeBuilder::emitE(LuauOpcode op, int32_t e)
{
    uint32_t insn = uint32_t(op) | (uint32_t(e) << 8);

    insns.push_back(insn);
    lines.push_back(debugLine);
}

void BytecodeBuilder::emitAux(uint32_t aux)
{
    insns.push_back(aux);
    lines.push_back(debugLine);
}

size_t BytecodeBuilder::emitLabel()
{
    return insns.size();
}

bool BytecodeBuilder::patchJumpD(size_t jumpLabel, size_t targetLabel)
{
    LUAU_ASSERT(jumpLabel < insns.size());

    unsigned int jumpInsn = insns[jumpLabel];
    (void)jumpInsn;

    LUAU_ASSERT(isJumpD(LuauOpcode(LUAU_INSN_OP(jumpInsn))));
    LUAU_ASSERT(LUAU_INSN_D(jumpInsn) == 0);

    LUAU_ASSERT(targetLabel <= insns.size());

    int offset = int(targetLabel) - int(jumpLabel) - 1;

    if (int16_t(offset) == offset)
    {
        insns[jumpLabel] |= uint16_t(offset) << 16;
    }
    else if (abs(offset) < kMaxJumpDistance)
    {
        // our jump doesn't fit into 16 bits; we will need to repatch the bytecode sequence with jump trampolines, see expandJumps
        hasLongJumps = true;
    }
    else
    {
        return false;
    }

    jumps.push_back({uint32_t(jumpLabel), uint32_t(targetLabel)});
    return true;
}

bool BytecodeBuilder::patchSkipC(size_t jumpLabel, size_t targetLabel)
{
    LUAU_ASSERT(jumpLabel < insns.size());

    unsigned int jumpInsn = insns[jumpLabel];
    (void)jumpInsn;

    LUAU_ASSERT(isSkipC(LuauOpcode(LUAU_INSN_OP(jumpInsn))));
    LUAU_ASSERT(LUAU_INSN_C(jumpInsn) == 0);

    int offset = int(targetLabel) - int(jumpLabel) - 1;

    if (uint8_t(offset) != offset)
    {
        return false;
    }

    insns[jumpLabel] |= offset << 24;
    return true;
}

void BytecodeBuilder::setDebugFunctionName(StringRef name)
{
    unsigned int index = addStringTableEntry(name);

    functions[currentFunction].debugname = index;

    if (dumpFunctionPtr)
        functions[currentFunction].dumpname = std::string(name.data, name.length);
}

void BytecodeBuilder::setDebugFunctionLineDefined(int line)
{
    functions[currentFunction].debuglinedefined = line;
}

void BytecodeBuilder::setDebugLine(int line)
{
    debugLine = line;
}

void BytecodeBuilder::pushDebugLocal(StringRef name, uint8_t reg, uint32_t startpc, uint32_t endpc)
{
    unsigned int index = addStringTableEntry(name);

    DebugLocal local;
    local.name = index;
    local.reg = reg;
    local.startpc = startpc;
    local.endpc = endpc;

    debugLocals.push_back(local);
}

void BytecodeBuilder::pushDebugUpval(StringRef name)
{
    unsigned int index = addStringTableEntry(name);

    DebugUpval upval;
    upval.name = index;

    debugUpvals.push_back(upval);
}

uint32_t BytecodeBuilder::getDebugPC() const
{
    return uint32_t(insns.size());
}

void BytecodeBuilder::finalize()
{
    LUAU_ASSERT(bytecode.empty());
    bytecode = char(LBC_VERSION_FUTURE);

    writeStringTable(bytecode);

    writeVarInt(bytecode, uint32_t(functions.size()));

    for (const Function& func : functions)
        bytecode += func.data;

    LUAU_ASSERT(mainFunction < functions.size());
    writeVarInt(bytecode, mainFunction);
}

void BytecodeBuilder::writeFunction(std::string& ss, uint32_t id) const
{
    LUAU_ASSERT(id < functions.size());
    const Function& func = functions[id];

    // header
    writeByte(ss, func.maxstacksize);
    writeByte(ss, func.numparams);
    writeByte(ss, func.numupvalues);
    writeByte(ss, func.isvararg);

    // instructions
    writeVarInt(ss, uint32_t(insns.size()));

    for (size_t i = 0; i < insns.size();)
    {
        uint8_t op = LUAU_INSN_OP(insns[i]);
        LUAU_ASSERT(op < LOP__COUNT);

        int oplen = getOpLength(LuauOpcode(op));
        uint8_t openc = encoder ? encoder->encodeOp(op) : op;

        writeInt(ss, openc | (insns[i] & ~0xff));

        for (int j = 1; j < oplen; ++j)
            writeInt(ss, insns[i + j]);

        i += oplen;
    }

    // constants
    writeVarInt(ss, uint32_t(constants.size()));

    for (const Constant& c : constants)
    {
        switch (c.type)
        {
        case Constant::Type_Nil:
            writeByte(ss, LBC_CONSTANT_NIL);
            break;

        case Constant::Type_Boolean:
            writeByte(ss, LBC_CONSTANT_BOOLEAN);
            writeByte(ss, c.valueBoolean);
            break;

        case Constant::Type_Number:
            writeByte(ss, LBC_CONSTANT_NUMBER);
            writeDouble(ss, c.valueNumber);
            break;

        case Constant::Type_String:
            writeByte(ss, LBC_CONSTANT_STRING);
            writeVarInt(ss, c.valueString);
            break;

        case Constant::Type_Import:
            writeByte(ss, LBC_CONSTANT_IMPORT);
            writeInt(ss, c.valueImport);
            break;

        case Constant::Type_Table:
        {
            const TableShape& shape = tableShapes[c.valueTable];
            writeByte(ss, LBC_CONSTANT_TABLE);
            writeVarInt(ss, uint32_t(shape.length));
            for (unsigned int i = 0; i < shape.length; ++i)
                writeVarInt(ss, shape.keys[i]);
            break;
        }

        case Constant::Type_Closure:
            writeByte(ss, LBC_CONSTANT_CLOSURE);
            writeVarInt(ss, c.valueClosure);
            break;

        default:
            LUAU_ASSERT(!xor("Unsupported constant type"));
        }
    }

    // child protos
    writeVarInt(ss, uint32_t(protos.size()));

    for (uint32_t child : protos)
        writeVarInt(ss, child);

    // debug info
    if constexpr (true)
        writeVarInt(ss, func.debuglinedefined);

    writeVarInt(ss, func.debugname);

    bool hasLines = true;

    for (int line : lines)
        if (line == 0)
        {
            hasLines = false;
            break;
        }

    if (hasLines)
    {
        writeByte(ss, 1);

        writeLineInfo(ss);
    }
    else
    {
        writeByte(ss, 0);
    }

    bool hasDebug = !debugLocals.empty() || !debugUpvals.empty();

    if (hasDebug)
    {
        writeByte(ss, 1);

        writeVarInt(ss, uint32_t(debugLocals.size()));

        for (const DebugLocal& l : debugLocals)
        {
            writeVarInt(ss, l.name);
            writeVarInt(ss, l.startpc);
            writeVarInt(ss, l.endpc);
            writeByte(ss, l.reg);
        }

        writeVarInt(ss, uint32_t(debugUpvals.size()));

        for (const DebugUpval& l : debugUpvals)
        {
            writeVarInt(ss, l.name);
        }
    }
    else
    {
        writeByte(ss, 0);
    }
}

void BytecodeBuilder::writeLineInfo(std::string& ss) const
{
    // this function encodes lines inside each span as a 8-bit delta to span baseline
    // span is always a power of two; depending on the line info input, it may need to be as low as 1
    int span = 1 << 24;

    // first pass: determine span length
    for (size_t offset = 0; offset < lines.size(); offset += span)
    {
        size_t next = offset;

        int min = lines[offset];
        int max = lines[offset];

        for (; next < lines.size() && next < offset + span; ++next)
        {
            min = std::min(min, lines[next]);
            max = std::max(max, lines[next]);

            if (max - min > 255)
                break;
        }

        if (next < lines.size() && next - offset < size_t(span))
        {
            // since not all lines in the range fit in 8b delta, we need to shrink the span
            // next iteration will need to reprocess some lines again since span changed
            span = 1 << log2(int(next - offset));
        }
    }

    // second pass: compute span base
    std::vector<int> baseline((lines.size() - 1) / span + 1);

    for (size_t offset = 0; offset < lines.size(); offset += span)
    {
        size_t next = offset;

        int min = lines[offset];

        for (; next < lines.size() && next < offset + span; ++next)
            min = std::min(min, lines[next]);

        baseline[offset / span] = min;
    }

    // third pass: write resulting data
    int logspan = log2(span);

    writeByte(ss, uint8_t(logspan));

    uint8_t lastOffset = 0;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        int delta = lines[i] - baseline[i >> logspan];
        LUAU_ASSERT(delta >= 0 && delta <= 255);

        writeByte(ss, uint8_t(delta) - lastOffset);
        lastOffset = uint8_t(delta);
    }

    int lastLine = 0;

    for (size_t i = 0; i < baseline.size(); ++i)
    {
        writeInt(ss, baseline[i] - lastLine);
        lastLine = baseline[i];
    }
}

void BytecodeBuilder::writeStringTable(std::string& ss) const
{
    std::vector<StringRef> strings(stringTable.size());

    for (auto& p : stringTable)
    {
        LUAU_ASSERT(p.second > 0 && p.second <= strings.size());
        strings[p.second - 1] = p.first;
    }

    writeVarInt(ss, uint32_t(strings.size()));

    for (auto& s : strings)
    {
        writeVarInt(ss, uint32_t(s.length));
        ss.append(s.data, s.length);
    }
}

uint32_t BytecodeBuilder::getImportId(int32_t id0)
{
    LUAU_ASSERT(unsigned(id0) < 1024);

    return (1u << 30) | (id0 << 20);
}

uint32_t BytecodeBuilder::getImportId(int32_t id0, int32_t id1)
{
    LUAU_ASSERT(unsigned(id0 | id1) < 1024);

    return (2u << 30) | (id0 << 20) | (id1 << 10);
}

uint32_t BytecodeBuilder::getImportId(int32_t id0, int32_t id1, int32_t id2)
{
    LUAU_ASSERT(unsigned(id0 | id1 | id2) < 1024);

    return (3u << 30) | (id0 << 20) | (id1 << 10) | id2;
}

uint32_t BytecodeBuilder::getStringHash(StringRef key)
{
    // This hashing algorithm should match luaS_hash defined in VM/lstring.cpp for short inputs; we can't use that code directly to keep compiler and
    // VM independent in terms of compilation/linking. The resulting string hashes are embedded into bytecode binary and result in a better initial
    // guess for the field hashes which improves performance during initial code execution. We omit the long string processing here for simplicity, as
    // it doesn't really matter on long identifiers.
    const char* str = key.data;
    size_t len = key.length;

    unsigned int h = unsigned(len);

    // original Lua 5.1 hash for compatibility (exact match when len<32)
    for (size_t i = len; i > 0; --i)
        h ^= (h << 5) + (h >> 2) + (uint8_t)str[i - 1];

    return h;
}

void BytecodeBuilder::foldJumps()
{
    // if our function has long jumps, some processing below can make jump instructions not-jumps (e.g. JUMP->RETURN)
    // it's safer to skip this processing
    if (hasLongJumps)
        return;

    for (Jump& jump : jumps)
    {
        uint32_t jumpLabel = jump.source;

        uint32_t jumpInsn = insns[jumpLabel];

        // follow jump target through forward unconditional jumps
        // we only follow forward jumps to make sure the process terminates
        uint32_t targetLabel = jumpLabel + 1 + LUAU_INSN_D(jumpInsn);
        LUAU_ASSERT(targetLabel < insns.size());
        uint32_t targetInsn = insns[targetLabel];

        while (LUAU_INSN_OP(targetInsn) == LOP_JUMP && LUAU_INSN_D(targetInsn) >= 0)
        {
            targetLabel = targetLabel + 1 + LUAU_INSN_D(targetInsn);
            LUAU_ASSERT(targetLabel < insns.size());
            targetInsn = insns[targetLabel];
        }

        int offset = int(targetLabel) - int(jumpLabel) - 1;

        // for unconditional jumps to RETURN, we can replace JUMP with RETURN
        if (LUAU_INSN_OP(jumpInsn) == LOP_JUMP && LUAU_INSN_OP(targetInsn) == LOP_RETURN)
        {
            insns[jumpLabel] = targetInsn;
            lines[jumpLabel] = lines[targetLabel];
        }
        else if (int16_t(offset) == offset)
        {
            insns[jumpLabel] &= 0xffff;
            insns[jumpLabel] |= uint16_t(offset) << 16;
        }

        jump.target = targetLabel;
    }
}

void BytecodeBuilder::expandJumps()
{
    if (!hasLongJumps)
        return;

    // we have some jump instructions that couldn't be patched which means their offset didn't fit into 16 bits
    // our strategy for replacing instructions is as follows: instead of
    //   OP jumpoffset
    // we will synthesize a jump trampoline before our instruction (note that jump offsets are relative to next instruction):
    //   JUMP +1
    //   JUMPX jumpoffset
    //   OP -2
    // the idea is that during forward execution, we will jump over JUMPX into OP; if OP decides to jump, it will jump to JUMPX
    // JUMPX can carry a 24-bit jump offset

    // jump trampolines expand the code size, which can increase existing jump distances.
    // because of this, we may need to expand jumps that previously fit into 16-bit just fine.
    // the worst-case expansion is 3x, so to be conservative we will repatch all jumps that have an offset >= 32767/3
    const int kMaxJumpDistanceConservative = 32767 / 3;

    // we will need to process jumps in order
    std::sort(jumps.begin(), jumps.end(), [](const Jump& lhs, const Jump& rhs) {
        return lhs.source < rhs.source;
    });

    // first, let's add jump thunks for every jump with a distance that's too big
    // we will create new instruction buffers, with remap table keeping track of the moves: remap[oldpc] = newpc
    std::vector<uint32_t> remap(insns.size());

    std::vector<uint32_t> newinsns;
    std::vector<int> newlines;

    LUAU_ASSERT(insns.size() == lines.size());
    newinsns.reserve(insns.size());
    newlines.reserve(insns.size());

    size_t currentJump = 0;
    size_t pendingTrampolines = 0;

    for (size_t i = 0; i < insns.size();)
    {
        uint8_t op = LUAU_INSN_OP(insns[i]);
        LUAU_ASSERT(op < LOP__COUNT);

        if (currentJump < jumps.size() && jumps[currentJump].source == i)
        {
            int offset = int(jumps[currentJump].target) - int(jumps[currentJump].source) - 1;

            if (abs(offset) > kMaxJumpDistanceConservative)
            {
                // insert jump trampoline as described above; we keep JUMPX offset uninitialized in this pass
                newinsns.push_back(LOP_JUMP | (1 << 16));
                newinsns.push_back(LOP_JUMPX);

                newlines.push_back(lines[i]);
                newlines.push_back(lines[i]);

                pendingTrampolines++;
            }

            currentJump++;
        }

        int oplen = getOpLength(LuauOpcode(op));

        // copy instruction and line info to the new stream
        for (int j = 0; j < oplen; ++j)
        {
            remap[i] = uint32_t(newinsns.size());

            newinsns.push_back(insns[i]);
            newlines.push_back(lines[i]);

            i++;
        }
    }

    LUAU_ASSERT(currentJump == jumps.size());
    LUAU_ASSERT(pendingTrampolines > 0);

    // now we need to recompute offsets for jump instructions - we could not do this in the first pass because the offsets are between *target*
    // instructions
    for (Jump& jump : jumps)
    {
        int offset = int(jump.target) - int(jump.source) - 1;
        int newoffset = int(remap[jump.target]) - int(remap[jump.source]) - 1;

        if (abs(offset) > kMaxJumpDistanceConservative)
        {
            // fix up jump trampoline
            uint32_t& insnt = newinsns[remap[jump.source] - 1];
            uint32_t& insnj = newinsns[remap[jump.source]];

            LUAU_ASSERT(LUAU_INSN_OP(insnt) == LOP_JUMPX);

            // patch JUMPX to JUMPX to target location; note that newoffset is the offset of the jump *relative to OP*, so we need to add 1 to make it
            // relative to JUMPX
            insnt &= 0xff;
            insnt |= uint32_t(newoffset + 1) << 8;

            // patch OP to OP -2
            insnj &= 0xffff;
            insnj |= uint16_t(-2) << 16;

            pendingTrampolines--;
        }
        else
        {
            uint32_t& insn = newinsns[remap[jump.source]];

            // make sure jump instruction had the correct offset before we started
            LUAU_ASSERT(LUAU_INSN_D(insn) == offset);

            // patch instruction with the new offset
            LUAU_ASSERT(int16_t(newoffset) == newoffset);

            insn &= 0xffff;
            insn |= uint16_t(newoffset) << 16;
        }
    }

    LUAU_ASSERT(pendingTrampolines == 0);

    // this was hard, but we're done.
    insns.swap(newinsns);
    lines.swap(newlines);
}

std::string BytecodeBuilder::getError(const std::string& message)
{
    // 0 acts as a special marker for error bytecode (it's equal to LBC_VERSION for valid bytecode blobs)
    std::string result;
    result += char(0);
    result += message;

    return result;
}

#ifdef LUAU_ASSERTENABLED
void BytecodeBuilder::validate() const
{
#define VREG(v) LUAU_ASSERT(unsigned(v) < func.maxstacksize)
#define VREGRANGE(v, count) LUAU_ASSERT(unsigned(v + (count < 0 ? 0 : count)) <= func.maxstacksize)
#define VUPVAL(v) LUAU_ASSERT(unsigned(v) < func.numupvalues)
#define VCONST(v, kind) LUAU_ASSERT(unsigned(v) < constants.size() && constants[v].type == Constant::Type_##kind)
#define VCONSTANY(v) LUAU_ASSERT(unsigned(v) < constants.size())
#define VJUMP(v) LUAU_ASSERT(size_t(i + 1 + v) < insns.size() && insnvalid[i + 1 + v])

    LUAU_ASSERT(currentFunction != ~0u);

    const Function& func = functions[currentFunction];

    // first pass: tag instruction offsets so that we can validate jumps
    std::vector<uint8_t> insnvalid(insns.size(), false);

    for (size_t i = 0; i < insns.size();)
    {
        uint8_t op = LUAU_INSN_OP(insns[i]);

        insnvalid[i] = true;

        i += getOpLength(LuauOpcode(op));
        LUAU_ASSERT(i <= insns.size());
    }

    // second pass: validate the rest of the bytecode
    for (size_t i = 0; i < insns.size();)
    {
        uint32_t insn = insns[i];
        uint8_t op = LUAU_INSN_OP(insn);

        switch (op)
        {
        case LOP_LOADNIL:
            VREG(LUAU_INSN_A(insn));
            break;

        case LOP_LOADB:
            VREG(LUAU_INSN_A(insn));
            LUAU_ASSERT(LUAU_INSN_B(insn) == 0 || LUAU_INSN_B(insn) == 1);
            VJUMP(LUAU_INSN_C(insn));
            break;

        case LOP_LOADN:
            VREG(LUAU_INSN_A(insn));
            break;

        case LOP_LOADK:
            VREG(LUAU_INSN_A(insn));
            VCONSTANY(LUAU_INSN_D(insn));
            break;

        case LOP_MOVE:
            VREG(LUAU_INSN_A(insn));
            VREG(LUAU_INSN_B(insn));
            break;

        case LOP_GETGLOBAL:
        case LOP_SETGLOBAL:
            VREG(LUAU_INSN_A(insn));
            VCONST(insns[i + 1], String);
            break;

        case LOP_GETUPVAL:
        case LOP_SETUPVAL:
            VREG(LUAU_INSN_A(insn));
            VUPVAL(LUAU_INSN_B(insn));
            break;

        case LOP_CLOSEUPVALS:
            VREG(LUAU_INSN_A(insn));
            break;

        case LOP_GETIMPORT:
            VREG(LUAU_INSN_A(insn));
            VCONST(LUAU_INSN_D(insn), Import);
            // TODO: check insn[i + 1] for conformance with 10-bit import encoding
            break;

        case LOP_GETTABLE:
        case LOP_SETTABLE:
            VREG(LUAU_INSN_A(insn));
            VREG(LUAU_INSN_B(insn));
            VREG(LUAU_INSN_C(insn));
            break;

        case LOP_GETTABLEKS:
        case LOP_SETTABLEKS:
            VREG(LUAU_INSN_A(insn));
            VREG(LUAU_INSN_B(insn));
            VCONST(insns[i + 1], String);
            break;

        case LOP_GETTABLEN:
        case LOP_SETTABLEN:
            VREG(LUAU_INSN_A(insn));
            VREG(LUAU_INSN_B(insn));
            break;

        case LOP_NEWCLOSURE:
        {
            VREG(LUAU_INSN_A(insn));
            LUAU_ASSERT(unsigned(LUAU_INSN_D(insn)) < protos.size());
            LUAU_ASSERT(protos[LUAU_INSN_D(insn)] < functions.size());
            unsigned int numupvalues = functions[protos[LUAU_INSN_D(insn)]].numupvalues;

            for (unsigned int j = 0; j < numupvalues; ++j)
            {
                LUAU_ASSERT(i + 1 + j < insns.size());
                uint32_t cinsn = insns[i + 1 + j];
                LUAU_ASSERT(LUAU_INSN_OP(cinsn) == LOP_CAPTURE);
            }
        }
        break;

        case LOP_NAMECALL:
            VREG(LUAU_INSN_A(insn));
            VREG(LUAU_INSN_B(insn));
            VCONST(insns[i + 1], String);
            LUAU_ASSERT(LUAU_INSN_OP(insns[i + 2]) == LOP_CALL);
            break;

        case LOP_CALL:
        {
            int nparams = LUAU_INSN_B(insn) - 1;
            int nresults = LUAU_INSN_C(insn) - 1;
            VREG(LUAU_INSN_A(insn));
            VREGRANGE(LUAU_INSN_A(insn) + 1, nparams); // 1..nparams
            VREGRANGE(LUAU_INSN_A(insn), nresults);    // 1..nresults
        }
        break;

        case LOP_RETURN:
        {
            int nresults = LUAU_INSN_B(insn) - 1;
            VREGRANGE(LUAU_INSN_A(insn), nresults); // 0..nresults-1
        }
        break;

        case LOP_JUMP:
            VJUMP(LUAU_INSN_D(insn));
            break;

        case LOP_JUMPIF:
        case LOP_JUMPIFNOT:
            VREG(LUAU_INSN_A(insn));
            VJUMP(LUAU_INSN_D(insn));
            break;

        case LOP_JUMPIFEQ:
        case LOP_JUMPIFLE:
        case LOP_JUMPIFLT:
        case LOP_JUMPIFNOTEQ:
        case LOP_JUMPIFNOTLE:
        case LOP_JUMPIFNOTLT:
            VREG(LUAU_INSN_A(insn));
            VREG(insns[i + 1]);
            VJUMP(LUAU_INSN_D(insn));
            break;

        case LOP_JUMPIFEQK:
        case LOP_JUMPIFNOTEQK:
            VREG(LUAU_INSN_A(insn));
            VCONSTANY(insns[i + 1]);
            VJUMP(LUAU_INSN_D(insn));
            break;

        case LOP_ADD:
        case LOP_SUB:
        case LOP_MUL:
        case LOP_DIV:
        case LOP_MOD:
        case LOP_POW:
            VREG(LUAU_INSN_A(insn));
            VREG(LUAU_INSN_B(insn));
            VREG(LUAU_INSN_C(insn));
            break;

        case LOP_ADDK:
        case LOP_SUBK:
        case LOP_MULK:
        case LOP_DIVK:
        case LOP_MODK:
        case LOP_POWK:
            VREG(LUAU_INSN_A(insn));
            VREG(LUAU_INSN_B(insn));
            VCONST(LUAU_INSN_C(insn), Number);
            break;

        case LOP_AND:
        case LOP_OR:
            VREG(LUAU_INSN_A(insn));
            VREG(LUAU_INSN_B(insn));
            VREG(LUAU_INSN_C(insn));
            break;

        case LOP_ANDK:
        case LOP_ORK:
            VREG(LUAU_INSN_A(insn));
            VREG(LUAU_INSN_B(insn));
            VCONSTANY(LUAU_INSN_C(insn));
            break;

        case LOP_CONCAT:
            VREG(LUAU_INSN_A(insn));
            VREG(LUAU_INSN_B(insn));
            VREG(LUAU_INSN_C(insn));
            LUAU_ASSERT(LUAU_INSN_B(insn) <= LUAU_INSN_C(insn));
            break;

        case LOP_NOT:
        case LOP_MINUS:
        case LOP_LENGTH:
            VREG(LUAU_INSN_A(insn));
            VREG(LUAU_INSN_B(insn));
            break;

        case LOP_NEWTABLE:
            VREG(LUAU_INSN_A(insn));
            break;

        case LOP_DUPTABLE:
            VREG(LUAU_INSN_A(insn));
            VCONST(LUAU_INSN_D(insn), Table);
            break;

        case LOP_SETLIST:
        {
            int count = LUAU_INSN_C(insn) - 1;
            VREG(LUAU_INSN_A(insn));
            VREGRANGE(LUAU_INSN_B(insn), count);
        }
        break;

        case LOP_FORNPREP:
        case LOP_FORNLOOP:
            VREG(LUAU_INSN_A(insn) + 2); // for loop protocol: A, A+1, A+2 are used for iteration
            VJUMP(LUAU_INSN_D(insn));
            break;

        case LOP_FORGLOOP:
            VREG(
                LUAU_INSN_A(insn) + 2 + insns[i + 1]); // forg loop protocol: A, A+1, A+2 are used for iteration protocol; A+3, ... are loop variables
            VJUMP(LUAU_INSN_D(insn));
            LUAU_ASSERT(insns[i + 1] >= 1);
            break;

        case LOP_FORGPREP_INEXT:
        case LOP_FORGLOOP_INEXT:
        case LOP_FORGPREP_NEXT:
        case LOP_FORGLOOP_NEXT:
            VREG(LUAU_INSN_A(insn) + 4); // forg loop protocol: A, A+1, A+2 are used for iteration protocol; A+3, A+4 are loop variables
            VJUMP(LUAU_INSN_D(insn));
            break;

        case LOP_GETVARARGS:
        {
            int nresults = LUAU_INSN_B(insn) - 1;
            VREGRANGE(LUAU_INSN_A(insn), nresults); // 0..nresults-1
        }
        break;

        case LOP_DUPCLOSURE:
        {
            VREG(LUAU_INSN_A(insn));
            VCONST(LUAU_INSN_D(insn), Closure);
            unsigned int proto = constants[LUAU_INSN_D(insn)].valueClosure;
            LUAU_ASSERT(proto < functions.size());
            unsigned int numupvalues = functions[proto].numupvalues;

            for (unsigned int j = 0; j < numupvalues; ++j)
            {
                LUAU_ASSERT(i + 1 + j < insns.size());
                uint32_t cinsn = insns[i + 1 + j];
                LUAU_ASSERT(LUAU_INSN_OP(cinsn) == LOP_CAPTURE);
                LUAU_ASSERT(LUAU_INSN_A(cinsn) == LCT_VAL || LUAU_INSN_A(cinsn) == LCT_UPVAL);
            }
        }
        break;

        case LOP_PREPVARARGS:
            LUAU_ASSERT(LUAU_INSN_A(insn) == func.numparams);
            LUAU_ASSERT(func.isvararg);
            break;

        case LOP_BREAK:
            break;

        case LOP_JUMPBACK:
            VJUMP(LUAU_INSN_D(insn));
            break;

        case LOP_LOADKX:
            VREG(LUAU_INSN_A(insn));
            VCONSTANY(insns[i + 1]);
            break;

        case LOP_JUMPX:
            VJUMP(LUAU_INSN_E(insn));
            break;

        case LOP_FASTCALL:
            VJUMP(LUAU_INSN_C(insn));
            LUAU_ASSERT(LUAU_INSN_OP(insns[i + 1 + LUAU_INSN_C(insn)]) == LOP_CALL);
            break;

        case LOP_FASTCALL1:
            VREG(LUAU_INSN_B(insn));
            VJUMP(LUAU_INSN_C(insn));
            LUAU_ASSERT(LUAU_INSN_OP(insns[i + 1 + LUAU_INSN_C(insn)]) == LOP_CALL);
            break;

        case LOP_FASTCALL2:
            VREG(LUAU_INSN_B(insn));
            VJUMP(LUAU_INSN_C(insn));
            LUAU_ASSERT(LUAU_INSN_OP(insns[i + 1 + LUAU_INSN_C(insn)]) == LOP_CALL);
            VREG(insns[i + 1]);
            break;

        case LOP_FASTCALL2K:
            VREG(LUAU_INSN_B(insn));
            VJUMP(LUAU_INSN_C(insn));
            LUAU_ASSERT(LUAU_INSN_OP(insns[i + 1 + LUAU_INSN_C(insn)]) == LOP_CALL);
            VCONSTANY(insns[i + 1]);
            break;

        case LOP_COVERAGE:
            break;

        case LOP_CAPTURE:
            switch (LUAU_INSN_A(insn))
            {
            case LCT_VAL:
            case LCT_REF:
                VREG(LUAU_INSN_B(insn));
                break;

            case LCT_UPVAL:
                VUPVAL(LUAU_INSN_B(insn));
                break;

            default:
                LUAU_ASSERT(!"Unsupported capture type");
            }
            break;

        default:
            LUAU_ASSERT(!"Unsupported opcode");
        }

        i += getOpLength(LuauOpcode(op));
        LUAU_ASSERT(i <= insns.size());
    }

#undef VREG
#undef VREGEND
#undef VUPVAL
#undef VCONST
#undef VCONSTANY
#undef VJUMP
}
#endif

const uint32_t* BytecodeBuilder::dumpInstruction(const uint32_t* code, std::string& result) const
{
    uint32_t insn = *code++;

    switch (LUAU_INSN_OP(insn))
    {
    case LOP_LOADNIL:
        formatAppend(result, xor("LOADNIL R%d\n"), LUAU_INSN_A(insn));
        break;

    case LOP_LOADB:
        if (LUAU_INSN_C(insn))
            formatAppend(result, xor("LOADB R%d %d +%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        else
            formatAppend(result, xor("LOADB R%d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn));
        break;

    case LOP_LOADN:
        formatAppend(result, xor("LOADN R%d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_LOADK:
        formatAppend(result, xor("LOADK R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_MOVE:
        formatAppend(result, xor("MOVE R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn));
        break;

    case LOP_GETGLOBAL:
        formatAppend(result, xor("GETGLOBAL R%d K%d\n"), LUAU_INSN_A(insn), *code++);
        break;

    case LOP_SETGLOBAL:
        formatAppend(result, xor("SETGLOBAL R%d K%d\n"), LUAU_INSN_A(insn), *code++);
        break;

    case LOP_GETUPVAL:
        formatAppend(result, xor("GETUPVAL R%d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn));
        break;

    case LOP_SETUPVAL:
        formatAppend(result, xor("SETUPVAL R%d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn));
        break;

    case LOP_CLOSEUPVALS:
        formatAppend(result, xor("CLOSEUPVALS R%d\n"), LUAU_INSN_A(insn));
        break;

    case LOP_GETIMPORT:
        formatAppend(result, xor("GETIMPORT R%d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        code++; // AUX
        break;

    case LOP_GETTABLE:
        formatAppend(result, xor("GETTABLE R%d R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_SETTABLE:
        formatAppend(result, xor("SETTABLE R%d R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_GETTABLEKS:
        formatAppend(result, xor("GETTABLEKS R%d R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), *code++);
        break;

    case LOP_SETTABLEKS:
        formatAppend(result, xor("SETTABLEKS R%d R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), *code++);
        break;

    case LOP_GETTABLEN:
        formatAppend(result, xor("GETTABLEN R%d R%d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn) + 1);
        break;

    case LOP_SETTABLEN:
        formatAppend(result, xor("SETTABLEN R%d R%d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn) + 1);
        break;

    case LOP_NEWCLOSURE:
        formatAppend(result, xor("NEWCLOSURE R%d P%d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_NAMECALL:
        formatAppend(result, xor("NAMECALL R%d R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), *code++);
        break;

    case LOP_CALL:
        formatAppend(result, xor("CALL R%d %d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn) - 1, LUAU_INSN_C(insn) - 1);
        break;

    case LOP_RETURN:
        formatAppend(result, xor("RETURN R%d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn) - 1);
        break;

    case LOP_JUMP:
        formatAppend(result, xor("JUMP %+d\n"), LUAU_INSN_D(insn));
        break;

    case LOP_JUMPIF:
        formatAppend(result, xor("JUMPIF R%d %+d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_JUMPIFNOT:
        formatAppend(result, xor("JUMPIFNOT R%d %+d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_JUMPIFEQ:
        formatAppend(result, xor("JUMPIFEQ R%d R%d %+d\n"), LUAU_INSN_A(insn), *code++, LUAU_INSN_D(insn));
        break;

    case LOP_JUMPIFLE:
        formatAppend(result, xor("JUMPIFLE R%d R%d %+d\n"), LUAU_INSN_A(insn), *code++, LUAU_INSN_D(insn));
        break;

    case LOP_JUMPIFLT:
        formatAppend(result, xor("JUMPIFLT R%d R%d %+d\n"), LUAU_INSN_A(insn), *code++, LUAU_INSN_D(insn));
        break;

    case LOP_JUMPIFNOTEQ:
        formatAppend(result, xor("JUMPIFNOTEQ R%d R%d %+d\n"), LUAU_INSN_A(insn), *code++, LUAU_INSN_D(insn));
        break;

    case LOP_JUMPIFNOTLE:
        formatAppend(result, xor("JUMPIFNOTLE R%d R%d %+d\n"), LUAU_INSN_A(insn), *code++, LUAU_INSN_D(insn));
        break;

    case LOP_JUMPIFNOTLT:
        formatAppend(result, xor("JUMPIFNOTLT R%d R%d %+d\n"), LUAU_INSN_A(insn), *code++, LUAU_INSN_D(insn));
        break;

    case LOP_ADD:
        formatAppend(result, xor("ADD R%d R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_SUB:
        formatAppend(result, xor("SUB R%d R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_MUL:
        formatAppend(result, xor("MUL R%d R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_DIV:
        formatAppend(result, xor("DIV R%d R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_MOD:
        formatAppend(result, xor("MOD R%d R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_POW:
        formatAppend(result, xor("POW R%d R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_ADDK:
        formatAppend(result, xor("ADDK R%d R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_SUBK:
        formatAppend(result, xor("SUBK R%d R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_MULK:
        formatAppend(result, xor("MULK R%d R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_DIVK:
        formatAppend(result, xor("DIVK R%d R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_MODK:
        formatAppend(result, xor("MODK R%d R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_POWK:
        formatAppend(result, xor("POWK R%d R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_AND:
        formatAppend(result, xor("AND R%d R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_OR:
        formatAppend(result, xor("OR R%d R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_ANDK:
        formatAppend(result, xor("ANDK R%d R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_ORK:
        formatAppend(result, xor("ORK R%d R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_CONCAT:
        formatAppend(result, xor("CONCAT R%d R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;

    case LOP_NOT:
        formatAppend(result, xor("NOT R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn));
        break;

    case LOP_MINUS:
        formatAppend(result, xor("MINUS R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn));
        break;

    case LOP_LENGTH:
        formatAppend(result, xor("LENGTH R%d R%d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn));
        break;

    case LOP_NEWTABLE:
        formatAppend(result, xor("NEWTABLE R%d %d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn) == 0 ? 0 : 1 << (LUAU_INSN_B(insn) - 1), *code++);
        break;

    case LOP_DUPTABLE:
        formatAppend(result, xor("DUPTABLE R%d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_SETLIST:
        formatAppend(result, xor("SETLIST R%d R%d %d [%d]\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn) - 1, *code++);
        break;

    case LOP_FORNPREP:
        formatAppend(result, xor("FORNPREP R%d %+d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_FORNLOOP:
        formatAppend(result, xor("FORNLOOP R%d %+d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_FORGLOOP:
        formatAppend(result, xor("FORGLOOP R%d %+d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn), *code++);
        break;

    case LOP_FORGPREP_INEXT:
        formatAppend(result, xor("FORGPREP_INEXT R%d %+d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_FORGLOOP_INEXT:
        formatAppend(result, xor("FORGLOOP_INEXT R%d %+d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_FORGPREP_NEXT:
        formatAppend(result, xor("FORGPREP_NEXT R%d %+d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_FORGLOOP_NEXT:
        formatAppend(result, xor("FORGLOOP_NEXT R%d %+d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_GETVARARGS:
        formatAppend(result, xor("GETVARARGS R%d %d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn) - 1);
        break;

    case LOP_DUPCLOSURE:
        formatAppend(result, xor("DUPCLOSURE R%d K%d\n"), LUAU_INSN_A(insn), LUAU_INSN_D(insn));
        break;

    case LOP_BREAK:
        formatAppend(result, xor("BREAK\n"));
        break;

    case LOP_JUMPBACK:
        formatAppend(result, xor("JUMPBACK %+d\n"), LUAU_INSN_D(insn));
        break;

    case LOP_LOADKX:
        formatAppend(result, xor("LOADKX R%d K%d\n"), LUAU_INSN_A(insn), *code++);
        break;

    case LOP_JUMPX:
        formatAppend(result, xor("JUMPX %+d\n"), LUAU_INSN_E(insn));
        break;

    case LOP_FASTCALL:
        formatAppend(result, xor("FASTCALL %d %+d\n"), LUAU_INSN_A(insn), LUAU_INSN_C(insn));
        break;

    case LOP_FASTCALL1:
        formatAppend(result, xor("FASTCALL1 %d R%d %+d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), LUAU_INSN_C(insn));
        break;
    case LOP_FASTCALL2:
    {
        uint32_t aux = *code++;
        formatAppend(result, xor("FASTCALL2 %d R%d R%d %+d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), aux, LUAU_INSN_C(insn));
        break;
    }
    case LOP_FASTCALL2K:
    {
        uint32_t aux = *code++;
        formatAppend(result, xor("FASTCALL2K %d R%d K%d %+d\n"), LUAU_INSN_A(insn), LUAU_INSN_B(insn), aux, LUAU_INSN_C(insn));
        break;
    }

    case LOP_COVERAGE:
        formatAppend(result, xor("COVERAGE\n"));
        break;

    case LOP_CAPTURE:
        formatAppend(result, xor("CAPTURE %s %c%d\n"),
            LUAU_INSN_A(insn) == LCT_UPVAL ? xor("UPVAL") : LUAU_INSN_A(insn) == LCT_REF ? xor("REF") : LUAU_INSN_A(insn) == LCT_VAL ? xor("VAL") : "",
            LUAU_INSN_A(insn) == LCT_UPVAL ? 'U' : 'R', LUAU_INSN_B(insn));
        break;

    case LOP_JUMPIFEQK:
        formatAppend(result, xor("JUMPIFEQK R%d K%d %+d\n"), LUAU_INSN_A(insn), *code++, LUAU_INSN_D(insn));
        break;

    case LOP_JUMPIFNOTEQK:
        formatAppend(result, xor("JUMPIFNOTEQK R%d K%d %+d\n"), LUAU_INSN_A(insn), *code++, LUAU_INSN_D(insn));
        break;

    default:
        LUAU_ASSERT(!xor("Unsupported opcode"));
    }

    return code;
}

std::string BytecodeBuilder::dumpCurrentFunction() const
{
    if ((dumpFlags & Dump_Code) == 0)
        return std::string();

    const uint32_t* code = insns.data();
    const uint32_t* codeEnd = insns.data() + insns.size();

    int lastLine = -1;

    std::string result;

    if (dumpFlags & Dump_Locals)
    {
        for (size_t i = 0; i < debugLocals.size(); ++i)
        {
            const DebugLocal& l = debugLocals[i];

            LUAU_ASSERT(l.startpc < l.endpc);
            LUAU_ASSERT(l.startpc < lines.size());
            LUAU_ASSERT(l.endpc <= lines.size()); // endpc is exclusive in the debug info, but it's more intuitive to print inclusive data

            // it would be nice to emit name as well but it requires reverse lookup through stringtable
            formatAppend(result, xor("local %d: reg %d, start pc %d line %d, end pc %d line %d\n"), int(i), l.reg, l.startpc, lines[l.startpc],
                l.endpc - 1, lines[l.endpc - 1]);
        }
    }

    while (code != codeEnd)
    {
        uint8_t op = LUAU_INSN_OP(*code);

        if (op == LOP_PREPVARARGS)
        {
            // Don't emit function header in bytecode - it's used for call dispatching and doesn't contain "interesting" information
            code++;
            continue;
        }

        if (dumpFlags & Dump_Source)
        {
            int line = lines[code - insns.data()];

            if (line > 0 && line != lastLine)
            {
                LUAU_ASSERT(size_t(line - 1) < dumpSource.size());
                formatAppend(result, xor("%5d: %s\n"), line, dumpSource[line - 1].c_str());
                lastLine = line;
            }
        }

        if (dumpFlags & Dump_Lines)
        {
            formatAppend(result, xor("%d: "), lines[code - insns.data()]);
        }

        code = dumpInstruction(code, result);
    }

    return result;
}

void BytecodeBuilder::setDumpSource(const std::string& source)
{
    dumpSource.clear();

    std::string::size_type pos = 0;

    while (pos != std::string::npos)
    {
        std::string::size_type next = source.find('\n', pos);

        if (next == std::string::npos)
        {
            dumpSource.push_back(source.substr(pos));
            pos = next;
        }
        else
        {
            dumpSource.push_back(source.substr(pos, next - pos));
            pos = next + 1;
        }

        if (!dumpSource.back().empty() && dumpSource.back().back() == '\r')
            dumpSource.back().pop_back();
    }
}

std::string BytecodeBuilder::dumpFunction(uint32_t id) const
{
    LUAU_ASSERT(id < functions.size());

    return functions[id].dump;
}

std::string BytecodeBuilder::dumpEverything() const
{
    std::string result;

    for (size_t i = 0; i < functions.size(); ++i)
    {
        std::string debugname = functions[i].dumpname.empty() ? "??" : functions[i].dumpname;

        formatAppend(result, xor("Function %d (%s):\n"), int(i), debugname.c_str());

        result += functions[i].dump;
        result += "\n";
    }

    return result;
}

} // namespace Compiler
