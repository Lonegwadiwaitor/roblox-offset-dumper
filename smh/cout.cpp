#include "../pch/pch.h"
#include "cout.hpp"

// wtf is this arson

namespace con
{
    static const WORD bgMask(BACKGROUND_BLUE |
        BACKGROUND_GREEN |
        BACKGROUND_RED |
        BACKGROUND_INTENSITY);
    static const WORD fgMask(FOREGROUND_BLUE |
        FOREGROUND_GREEN |
        FOREGROUND_RED |
        FOREGROUND_INTENSITY);

    static const WORD fgBlack(0);
    static const WORD fgLoRed(FOREGROUND_RED);
    static const WORD fgLoGreen(FOREGROUND_GREEN);
    static const WORD fgLoBlue(FOREGROUND_BLUE);
    static const WORD fgLoCyan(fgLoGreen | fgLoBlue);
    static const WORD fgLoMagenta(fgLoRed | fgLoBlue);
    static const WORD fgLoYellow(fgLoRed | fgLoGreen);
    static const WORD fgLoWhite(fgLoRed | fgLoGreen | fgLoBlue);
    static const WORD fgNone(FOREGROUND_INTENSITY);
    static const WORD fgGray(fgBlack | FOREGROUND_INTENSITY);
    static const WORD fgHiWhite(fgLoWhite | FOREGROUND_INTENSITY);
    static const WORD fgHiBlue(fgLoBlue | FOREGROUND_INTENSITY);
    static const WORD fgHiGreen(fgLoGreen | FOREGROUND_INTENSITY);
    static const WORD fgHiRed(fgLoRed | FOREGROUND_INTENSITY);
    static const WORD fgHiCyan(fgLoCyan | FOREGROUND_INTENSITY);
    static const WORD fgHiMagenta(fgLoMagenta | FOREGROUND_INTENSITY);
    static const WORD fgHiYellow(fgLoYellow | FOREGROUND_INTENSITY);
    static const WORD bgBlack(0);
    static const WORD bgLoRed(BACKGROUND_RED);
    static const WORD bgLoGreen(BACKGROUND_GREEN);
    static const WORD bgLoBlue(BACKGROUND_BLUE);
    static const WORD bgLoCyan(bgLoGreen | bgLoBlue);
    static const WORD bgLoMagenta(bgLoRed | bgLoBlue);
    static const WORD bgLoYellow(bgLoRed | bgLoGreen);
    static const WORD bgLoWhite(bgLoRed | bgLoGreen | bgLoBlue);
    static const WORD bgGray(bgBlack | BACKGROUND_INTENSITY);
    static const WORD bgHiWhite(bgLoWhite | BACKGROUND_INTENSITY);
    static const WORD bgHiBlue(bgLoBlue | BACKGROUND_INTENSITY);
    static const WORD bgHiGreen(bgLoGreen | BACKGROUND_INTENSITY);
    static const WORD bgHiRed(bgLoRed | BACKGROUND_INTENSITY);
    static const WORD bgHiCyan(bgLoCyan | BACKGROUND_INTENSITY);
    static const WORD bgHiMagenta(bgLoMagenta | BACKGROUND_INTENSITY);
    static const WORD bgHiYellow(bgLoYellow | BACKGROUND_INTENSITY);

    static class con_dev
    {
    private:
        HANDLE                      hCon;
        DWORD                       cCharsWritten{};
        CONSOLE_SCREEN_BUFFER_INFO  csbi{};
        DWORD                       dwConSize{};

    public:
        con_dev() {
            AllocConsole();

            SetConsoleTitleW(L"based-code® generator™");
            freopen_s(&safe_handle_stream, xor("CONIN$"), xor("r"), stdin);
            freopen_s(&safe_handle_stream, xor("CONOUT$"), xor("w"), stdout);
            freopen_s(&safe_handle_stream, xor("CONOUT$"), xor("w"), stderr);

            hCon = GetStdHandle(STD_OUTPUT_HANDLE);  // NOLINT(cppcoreguidelines-prefer-member-initializer)
        }
    private:
        void GetInfo()
        {
            GetConsoleScreenBufferInfo(hCon, &csbi);
            dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
        }
    public:
        void Clear()
        {
            COORD coordScreen = { 0, 0 };

            GetInfo();
            FillConsoleOutputCharacter(hCon, ' ',
                dwConSize,
                coordScreen,
                &cCharsWritten);
            GetInfo();
            FillConsoleOutputAttribute(hCon,
                csbi.wAttributes,
                dwConSize,
                coordScreen,
                &cCharsWritten);
            SetConsoleCursorPosition(hCon, coordScreen);
        }
        void SetColor(WORD wRGBI, WORD Mask)
        {
            GetInfo();
            csbi.wAttributes &= Mask;
            csbi.wAttributes |= wRGBI;
            SetConsoleTextAttribute(hCon, csbi.wAttributes);
        }
    } console;

    std::ostream& fg_none(std::ostream& os)
    {
        os.flush();
        console.SetColor(fgLoWhite, bgMask);

        return os;
    }

    std::ostream& fg_red(std::ostream& os)
    {
        os.flush();
        console.SetColor(fgHiRed, bgMask);

        return os;
    }

    std::ostream& fg_green(std::ostream& os)
    {
        os.flush();
        console.SetColor(fgHiGreen, bgMask);

        return os;
    }

    std::ostream& fg_cyan(std::ostream& os)
    {
        os.flush();
        console.SetColor(fgHiCyan, bgMask);

        return os;
    }

    std::ostream& fg_magenta(std::ostream& os)
    {
        os.flush();
        console.SetColor(fgHiMagenta, bgMask);

        return os;
    }

    std::ostream& fg_yellow(std::ostream& os)
    {
        os.flush();
        console.SetColor(fgHiYellow, bgMask);

        return os;
    }
}
