#pragma once

#include <cstdint>

#include "structs.hpp"
#include "structs.hpp"

namespace roblox::shuffles
{
	namespace original_structs {
#define CommonHeader \
     uint8_t v1; uint8_t v2; uint8_t v3
		using StkId = std::uint32_t;
		using UpVal = std::uint32_t;
		using GCObject = std::uint32_t;
		using Instruction = std::uint16_t;
		using TValue = std::uint8_t[16];
		using lua_Alloc = std::uint32_t;
		using lua_Debug = std::uint32_t;
		using LuaNode = std::uint8_t[28];


		constexpr auto LUA_SIZECLASSES = 32;
		constexpr auto LUA_MEMORY_CATEGORIES = 256;
		constexpr auto LUA_T_COUNT = 10;
		constexpr auto TM_N = 19;
		constexpr auto LUA_UTAG_LIMIT = 128;

		struct lua_State;
		struct Table;
		struct TString;

		typedef struct CallInfo
		{

			StkId base;    /* base for this function */
			StkId func;    /* function index in the stack */
			StkId top;     /* top for this function */
			const Instruction* savedpc;

			int nresults;       /* expected number of results from this function */
			unsigned int flags; /* call frame flags, see LUA_CALLINFO_* */
		} CallInfo;

		typedef struct stringtable
		{

			TString** hash;
			uint32_t nuse; /* number of elements */
			int size;
		} stringtable;

		struct lua_Callbacks
		{
			void* userdata; /* arbitrary userdata pointer that is never overwritten by Luau */

			void (*interrupt)(lua_State* L, int gc);  /* gets called at safepoints (loop back edges, call/ret, gc) if set */
			void (*panic)(lua_State* L, int errcode); /* gets called when an unprotected error is raised (if longjmp is used) */

			void (*userthread)(lua_State* LP, lua_State* L); /* gets called when L is created (LP == parent) or destroyed (LP == NULL) */
			int16_t(*useratom)(const char* s, size_t l);    /* gets called when a string is created; returned atom can be retrieved via tostringatom */

			void (*debugbreak)(lua_State* L, lua_Debug* ar);     /* gets called when BREAK instruction is encountered */
			void (*debugstep)(lua_State* L, lua_Debug* ar);      /* gets called after each instruction in single step mode */
			void (*debuginterrupt)(lua_State* L, lua_Debug* ar); /* gets called when thread execution is interrupted by break in another thread */
			void (*debugprotectederror)(lua_State* L);           /* gets called when protected call results in an error */
		};
		typedef struct lua_Callbacks lua_Callbacks;

		struct GCStats
		{
			// data for proportional-integral controller of heap trigger value
			int32_t triggerterms[32] = { 0 };
			uint32_t triggertermpos = 0;
			int32_t triggerintegral = 0;

			size_t atomicstarttotalsizebytes = 0;
			size_t endtotalsizebytes = 0;
			size_t heapgoalsizebytes = 0;

			double starttimestamp = 0;
			double atomicstarttimestamp = 0;
			double endtimestamp = 0;
		};

		struct GCCycleMetrics
		{
			size_t starttotalsizebytes = 0;
			size_t heaptriggersizebytes = 0;

			double pausetime = 0.0; // time from end of the last cycle to the start of a new one

			double starttimestamp = 0.0;
			double endtimestamp = 0.0;

			double marktime = 0.0;
			double markassisttime = 0.0;
			double markmaxexplicittime = 0.0;
			size_t markexplicitsteps = 0;
			size_t markwork = 0;

			double atomicstarttimestamp = 0.0;
			size_t atomicstarttotalsizebytes = 0;
			double atomictime = 0.0;

			// specific atomic stage parts
			double atomictimeupval = 0.0;
			double atomictimeweak = 0.0;
			double atomictimegray = 0.0;
			double atomictimeclear = 0.0;

			double sweeptime = 0.0;
			double sweepassisttime = 0.0;
			double sweepmaxexplicittime = 0.0;
			size_t sweepexplicitsteps = 0;
			size_t sweepwork = 0;

			size_t assistwork = 0;
			size_t explicitwork = 0;

			size_t propagatework = 0;
			size_t propagateagainwork = 0;

			size_t endtotalsizebytes = 0;
		};

		struct GCMetrics
		{
			double stepexplicittimeacc = 0.0;
			double stepassisttimeacc = 0.0;

			// when cycle is completed, last cycle values are updated
			uint64_t completedcycles = 0;

			GCCycleMetrics lastcycle;
			GCCycleMetrics currcycle;
		};

		typedef struct Table
		{
			CommonHeader;


			uint8_t tmcache;    /* 1<<p means tagmethod(p) is not present */
			uint8_t readonly;   /* sandboxing feature to prohibit writes to table */
			uint8_t safeenv;    /* environment doesn't share globals with other scripts */
			uint8_t lsizenode;  /* log2 of size of `node' array */
			uint8_t nodemask8; /* (1<<lsizenode)-1, truncated to 8 bits */

			int sizearray; /* size of `array' array */
			union
			{
				int lastfree;  /* any free position is before this position */
				int aboundary; /* negated 'boundary' of `array' array; iff aboundary < 0 */
			};


			struct Table* metatable;
			TValue* array;  /* array part */
			LuaNode* node;
			GCObject* gclist;
		} Table;

		typedef struct TString
		{
			CommonHeader;
			// 1 byte padding

			int16_t atom;
			// 2 byte padding

			TString* next; // next string in the hash table bucket or the string buffer linked list

			unsigned int hash;
			unsigned int len;

			char data[1]; // string data is allocated right after the header
		} TString;

		typedef struct Closure
		{
			CommonHeader;

			uint8_t isC;
			uint8_t nupvalues;
			uint8_t stacksize;
			uint8_t preload;

			GCObject* gclist;
			struct Table* env;

			union
			{
				struct
				{
					std::uintptr_t f;
					std::uintptr_t cont;
					const char* debugname;
					TValue upvals[1];
				} c;

				struct
				{
					struct Proto* p;
					TValue uprefs[1];
				} l;
			};
		} Closure;

#define LUAI_USER_ALIGNMENT_T \
    union \
    { \
        double u; \
        void* s; \
        long l; \
    }

		typedef LUAI_USER_ALIGNMENT_T L_Umaxalign;


		typedef struct Udata
		{
			CommonHeader;

			uint8_t tag;

			int len;

			struct Table* metatable;

			union
			{
				char data[1];      // userdata is allocated right after the header
				L_Umaxalign dummy; // ensures maximum alignment for data
			};
		} Udata;

		typedef struct global_State
		{
			stringtable strt; /* hash table for strings */


			lua_Alloc frealloc;   /* function to reallocate memory */
			void* ud;            /* auxiliary data to `frealloc' */


			uint8_t currentwhite;
			uint8_t gcstate; /* state of garbage collector */


			GCObject* gray;      /* list of gray objects */
			GCObject* grayagain; /* list of objects to be traversed atomically */
			GCObject* weak;     /* list of weak tables (to be cleared) */

			size_t GCthreshold;                       // when totalbytes > GCthreshold; run GC step
			size_t totalbytes;                        // number of bytes currently allocated
			int gcgoal;                               // see LUAI_GCGOAL
			int gcstepmul;                            // see LUAI_GCSTEPMUL
			int gcstepsize;                          // see LUAI_GCSTEPSIZE

			struct lua_Page* freepages[LUA_SIZECLASSES]; // free page linked list for each size class for non-collectable objects
			struct lua_Page* freegcopages[LUA_SIZECLASSES]; // free page linked list for each size class for collectable objects 
			struct lua_Page* allgcopages; // page linked list with all pages for all classes
			struct lua_Page* sweepgcopage; // position of the sweep in `allgcopages'

			size_t memcatbytes[LUA_MEMORY_CATEGORIES]; /* total amount of memory used by each memory category */


			struct lua_State* mainthread;
			UpVal uvhead;                                    /* head of double-linked list of all open upvalues */
			struct Table* mt[LUA_T_COUNT];                   /* metatables for basic types */
			TString* ttname[LUA_T_COUNT];       /* names for basic types */
			TString* tmname[TM_N];             /* array with tag-method names */

			TValue pseudotemp; /* storage for temporary values used in pseudo2addr */

			TValue registry; /* registry table, used by lua_ref and LUA_REGISTRYINDEX */
			int registryfree; /* next free slot in registry */

			struct lua_jmpbuf* errorjmp; /* jump buffer data for longjmp-style error handling */

			uint64_t rngstate; /* PCG random number generator state */
			uint64_t ptrenckey[4]; /* pointer encoding key for display */

			void (*udatagc[LUA_UTAG_LIMIT])(lua_State*, void*); /* for each userdata tag, a gc callback to be called immediately before freeing memory */

			lua_Callbacks cb;

			GCStats gcstats;

#ifdef LUAI_GCMETRICS
			GCMetrics gcmetrics;
#endif
		} global_State;

		struct lua_State
		{
			CommonHeader;
			uint8_t status;

			uint8_t activememcat; /* memory category that is used for new GC object allocations */
			uint8_t stackstate;

			bool singlestep; /* call debugstep hook after each instruction */


			StkId top;                                        /* first free slot in the stack */
			StkId base;                                       /* base of current function */
			global_State* global;
			CallInfo* ci;                                     /* call info for current function */
			StkId stack_last;                                 /* last free slot in the stack */
			StkId stack;                                     /* stack base */


			CallInfo* end_ci;                          /* points after end of ci array*/
			CallInfo* base_ci;                        /* array of CallInfo's */


			int stacksize;
			int size_ci;                              /* size of array `base_ci' */


			unsigned short nCcalls;     /* number of nested C calls */
			unsigned short baseCcalls; /* nested C calls when resuming coroutine */

			int cachedslot;    /* when table operations or INDEX/NEWINDEX is invoked from Luau, what is the expected slot for lookup? */


			Table* gt;           /* table of globals */
			UpVal* openupval;    /* list of open upvalues in this stack */
			GCObject* gclist;

			TString* namecall; /* when invoked from Luau using NAMECALL, what method do we need to invoke? */

			void* userdata;
		};

		typedef struct Proto
		{
			CommonHeader;


			TValue* k;              /* constants used by the function */
			Instruction* code;      /* function bytecode */
			struct Proto** p;       /* functions defined inside the function */
			uint8_t* lineinfo;      /* for each instruction, line number as a delta from baseline */
			int* abslineinfo;       /* baseline line info, one entry for each 1<<linegaplog2 instructions; allocated after lineinfo */
			struct LocVar* locvars; /* information about local variables */
			TString** upvalues;     /* upvalue names */
			TString* source;

			TString* debugname;
			uint8_t* debuginsn; // a copy of code[] array with just opcodes

			GCObject* gclist;


			int sizecode;
			int sizep;
			int sizelocvars;
			int sizeupvalues;
			int sizek;
			int sizelineinfo;
			int linegaplog2;
			int linedefined;


			uint8_t nups; /* number of upvalues */
			uint8_t numparams;
			uint8_t is_vararg;
			uint8_t maxstacksize;
		} Proto;

#undef CommonHeader
	}

	namespace new_structs {
		struct strt {
			std::uint16_t hash = offsetof(roblox::shuffles::original_structs::stringtable, hash);
			std::uint16_t size = offsetof(roblox::shuffles::original_structs::stringtable, nuse);
			std::uint16_t capacity = offsetof(roblox::shuffles::original_structs::stringtable, size);
		};

		struct gch {
			std::uint16_t type = 0;
			std::uint16_t marked = 1;
			std::uint16_t modkey = 2;
		};


		struct ls {
			std::uint16_t status = offsetof(roblox::shuffles::original_structs::lua_State, status);
			std::uint16_t activememcat = offsetof(roblox::shuffles::original_structs::lua_State, activememcat);
			std::uint16_t stackstate = offsetof(roblox::shuffles::original_structs::lua_State, stackstate);

			std::uint16_t singlestep = offsetof(roblox::shuffles::original_structs::lua_State, singlestep);

			std::uint16_t top = offsetof(roblox::shuffles::original_structs::lua_State, top);
			std::uint16_t base = offsetof(roblox::shuffles::original_structs::lua_State, base);
			std::uint16_t global = offsetof(roblox::shuffles::original_structs::lua_State, global);
			std::uint16_t ci = offsetof(roblox::shuffles::original_structs::lua_State, ci);
			std::uint16_t stack_last = offsetof(roblox::shuffles::original_structs::lua_State, stack_last);
			std::uint16_t stack = offsetof(roblox::shuffles::original_structs::lua_State, stack);

			std::uint16_t end_ci = offsetof(roblox::shuffles::original_structs::lua_State, end_ci);
			std::uint16_t base_ci = offsetof(roblox::shuffles::original_structs::lua_State, base_ci);

			int stacksize = offsetof(roblox::shuffles::original_structs::lua_State, stacksize);
			size_t size_ci = offsetof(roblox::shuffles::original_structs::lua_State, size_ci);

			std::uint16_t nCcalls = offsetof(roblox::shuffles::original_structs::lua_State, nCcalls);
			std::uint16_t baseCcalls = offsetof(roblox::shuffles::original_structs::lua_State, baseCcalls);

			int cachedslot = offsetof(roblox::shuffles::original_structs::lua_State, cachedslot);

			std::uint16_t gt = offsetof(roblox::shuffles::original_structs::lua_State, gt);
			std::uint16_t openupval = offsetof(roblox::shuffles::original_structs::lua_State, openupval);
			std::uint16_t gclist = offsetof(roblox::shuffles::original_structs::lua_State, gclist);

			std::uint16_t namecall = offsetof(roblox::shuffles::original_structs::lua_State, namecall);
			std::uint16_t userdata = offsetof(roblox::shuffles::original_structs::lua_State, userdata);
		};

		struct ci {
			std::uint16_t base = offsetof(roblox::shuffles::original_structs::CallInfo, base);
			std::uint16_t func = offsetof(roblox::shuffles::original_structs::CallInfo, func);
			std::uint16_t top = offsetof(roblox::shuffles::original_structs::CallInfo, top);
			std::uint16_t savedpc = offsetof(roblox::shuffles::original_structs::CallInfo, savedpc);
			std::uint16_t nresults = offsetof(roblox::shuffles::original_structs::CallInfo, nresults);
			std::uint16_t flags = offsetof(roblox::shuffles::original_structs::CallInfo, flags);
		};

		struct tbl {
			std::uint16_t flags = offsetof(roblox::shuffles::original_structs::Table, tmcache);
			std::uint16_t readonly = offsetof(roblox::shuffles::original_structs::Table, readonly);
			std::uint16_t safeenv = offsetof(roblox::shuffles::original_structs::Table, safeenv);
			std::uint16_t lsizenode = offsetof(roblox::shuffles::original_structs::Table, lsizenode);
			std::uint16_t nodemask8 = offsetof(roblox::shuffles::original_structs::Table, nodemask8);
			std::uint16_t sizearray = offsetof(roblox::shuffles::original_structs::Table, sizearray);
			std::uint16_t lastfree = offsetof(roblox::shuffles::original_structs::Table, lastfree);
			std::uint16_t metatable = offsetof(roblox::shuffles::original_structs::Table, metatable);
			std::uint16_t array = offsetof(roblox::shuffles::original_structs::Table, array);
			std::uint16_t node = offsetof(roblox::shuffles::original_structs::Table, node);
			std::uint16_t gclist = offsetof(roblox::shuffles::original_structs::Table, gclist);
		};

		struct global {
			std::uint16_t gray = offsetof(roblox::shuffles::original_structs::global_State, gray);
			std::uint16_t grayagain = offsetof(roblox::shuffles::original_structs::global_State, grayagain);
			std::uint16_t weak = offsetof(roblox::shuffles::original_structs::global_State, weak);

			std::uint16_t GCthreshold = offsetof(roblox::shuffles::original_structs::global_State, GCthreshold);
			std::uint16_t totalbytes = offsetof(roblox::shuffles::original_structs::global_State, totalbytes);
			std::uint16_t gcgoal = offsetof(roblox::shuffles::original_structs::global_State, gcgoal);
			std::uint16_t gcstepmul = offsetof(roblox::shuffles::original_structs::global_State, gcstepmul);
			std::uint16_t gcstepsize = offsetof(roblox::shuffles::original_structs::global_State, gcstepsize);

			std::uint16_t freepages = offsetof(roblox::shuffles::original_structs::global_State, freepages);
			std::uint16_t freegcopages = offsetof(roblox::shuffles::original_structs::global_State, freegcopages);
			std::uint16_t allgcopages = offsetof(roblox::shuffles::original_structs::global_State, allgcopages);
			std::uint16_t sweepgcopage = offsetof(roblox::shuffles::original_structs::global_State, sweepgcopage);

			std::uint16_t mainthread = offsetof(roblox::shuffles::original_structs::global_State, mainthread);
			std::uint16_t uvhead = offsetof(roblox::shuffles::original_structs::global_State, uvhead);
			std::uint16_t mt = offsetof(roblox::shuffles::original_structs::global_State, mt);
			std::uint16_t ttname = offsetof(roblox::shuffles::original_structs::global_State, ttname);
			std::uint16_t tmname = offsetof(roblox::shuffles::original_structs::global_State, tmname);
		};

		struct proto {
			std::uint16_t k = offsetof(roblox::shuffles::original_structs::Proto, k);
			std::uint16_t code = offsetof(roblox::shuffles::original_structs::Proto, code);
			std::uint16_t p = offsetof(roblox::shuffles::original_structs::Proto, p);
			std::uint16_t lineinfo = offsetof(roblox::shuffles::original_structs::Proto, lineinfo);
			std::uint16_t abslineinfo = offsetof(roblox::shuffles::original_structs::Proto, abslineinfo);
			std::uint16_t locvars = offsetof(roblox::shuffles::original_structs::Proto, locvars);
			std::uint16_t upvalues = offsetof(roblox::shuffles::original_structs::Proto, upvalues);
			std::uint16_t source = offsetof(roblox::shuffles::original_structs::Proto, source);

			std::uint16_t debugname = offsetof(roblox::shuffles::original_structs::Proto, debugname);

			std::uint16_t sizecode = offsetof(roblox::shuffles::original_structs::Proto, sizecode);
			std::uint16_t sizep = offsetof(roblox::shuffles::original_structs::Proto, sizep);
			std::uint16_t sizelocvars = offsetof(roblox::shuffles::original_structs::Proto, sizelocvars);
			std::uint16_t sizeupvalues = offsetof(roblox::shuffles::original_structs::Proto, sizeupvalues);
			std::uint16_t sizek = offsetof(roblox::shuffles::original_structs::Proto, sizek);
			std::uint16_t sizelineinfo = offsetof(roblox::shuffles::original_structs::Proto, sizelineinfo);
			std::uint16_t linegaplog2 = offsetof(roblox::shuffles::original_structs::Proto, linegaplog2);
			std::uint16_t linedefined = offsetof(roblox::shuffles::original_structs::Proto, linedefined);
		};
	}
}
