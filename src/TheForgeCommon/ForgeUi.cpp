#include "ForgeUi.h"

#include "ForgeLineUi.h"
#include "ForgeSpriteUi.h"

namespace {

// A fila de edges e o estado segurado agora sao MECANISMO DA ENGINE
// (cengine::input::Keyboard, task 20 / 0.8.0). Esta ponte so guarda a instancia
// e continua fazendo o que so ela pode fazer: capturar.
cengine::input::Keyboard gKeyboard;

Cmd*     gCmd = NULL;
float    gWidth = 0.0f;
float    gHeight = 0.0f;
uint32_t gFontID = 0;

} // namespace

namespace forgeui {

cengine::input::Keyboard& keyboard() { return gKeyboard; }

void pushKey(const KeyEvent event) { gKeyboard.pushKey(event); }

void pushHeldKey(const Key key, const bool held) { gKeyboard.pushHeldKey(key, held); }

void clearHeldKeys() { gKeyboard.clearHeldKeys(); }

void beginDraw(Cmd* cmd, const float width, const float height, const uint32_t fontID)
{
    gCmd = cmd;
    gWidth = width;
    gHeight = height;
    gFontID = fontID;
}

KeyEvent readKey() { return gKeyboard.readKey(); }

bool isHeld(const Key key) { return gKeyboard.isHeld(key); }

float heldAxis(const Key negative, const Key positive) { return gKeyboard.heldAxis(negative, positive); }

float screenWidth() { return gWidth; }
float screenHeight() { return gHeight; }

float textWidth(const std::string& text, const float fontSize)
{
    FontDrawDesc desc = {};
    desc.pText = text.c_str();
    desc.mFontID = gFontID;
    desc.mFontSize = fontSize;
    return fntMeasureFontText(desc.pText, &desc).x;
}

void drawText(const std::string& text, const float x, const float y, const float fontSize, const uint32_t colorAbgr)
{
    // Camadas = ordem de chamada atravessando as pontes: o que estiver pendente
    // nos batchers e desenhado AGORA, para este texto ficar por cima.
    forgesprite::flush();
    forgeline::flush();

    FontDrawDesc desc = {};
    desc.pText = text.c_str();
    desc.mFontID = gFontID;
    desc.mFontColor = colorAbgr;
    desc.mFontSize = fontSize;
    cmdDrawTextWithFont(gCmd, float2(x, y), &desc);
}

void drawTextCentered(const std::string& text, const float y, const float fontSize, const uint32_t colorAbgr)
{
    drawText(text, (gWidth - textWidth(text, fontSize)) * 0.5f, y, fontSize, colorAbgr);
}

void drawHints(const std::string& text) { drawTextCentered(text, gHeight - 48.0f, 18.0f, color::kDim); }

} // namespace forgeui
