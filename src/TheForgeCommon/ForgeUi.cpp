#include "ForgeUi.h"

#include <cstddef>
#include <vector>

#include "ForgeLineUi.h"
#include "ForgeSpriteUi.h"

namespace {

// A fila de edges e o estado segurado agora sao MECANISMO DA ENGINE
// (cengine::input::Keyboard, task 20 / 0.8.0). Esta ponte so guarda a instancia
// e continua fazendo o que so ela pode fazer: capturar.
cengine::input::Keyboard gKeyboard;

// O ponteiro tambem virou MECANISMO DA ENGINE (cengine::input::Mouse, task 27
// / 0.14.0), pelo mesmo caminho que o teclado fez na 0.8.0: viveu aqui
// enquanto tinha um consumidor so, e subiu quando o segundo usou a forma do
// primeiro sem mudar nada. A ponte segue guardando a instancia e capturando.
cengine::input::Mouse gMouse;

// O ARRASTAR ainda nao tem porta na engine (ver ForgeUi.h): um consumidor so.
// Estado e fila vivem aqui, com a mesma disciplina do resto — o gesto em
// andamento e estado continuo, o gesto terminado e edge.
forgeui::DragState gDrag;
std::vector<forgeui::Drop> gDrops;

// Mesmo teto das outras filas, e pelo mesmo motivo: se ninguem consome, ela
// nao cresce sem limite.
constexpr size_t kDropQueueMax = 16;

Cmd*     gCmd = NULL;
float    gWidth = 0.0f;
float    gHeight = 0.0f;
uint32_t gFontID = 0;

} // namespace

namespace forgeui {

cengine::input::Keyboard& keyboard() { return gKeyboard; }

cengine::input::Mouse& mouse() { return gMouse; }

void pushKey(const KeyEvent event) { gKeyboard.pushKey(event); }

void pushHeldKey(const Key key, const bool held) { gKeyboard.pushHeldKey(key, held); }

void clearHeldKeys() { gKeyboard.clearHeldKeys(); }

void pushMousePosition(const float x, const float y)
{
    gMouse.pushPosition(x, y);

    // O gesto em andamento acompanha o ponteiro: e o que permite a cena
    // desenhar o que esta na mao seguindo o cursor.
    if (gDrag.active)
    {
        gDrag.x = x;
        gDrag.y = y;
    }
}

void pushMouseClick(const MouseClick click) { gMouse.pushClick(click); }

void pushMouseDown(const float x, const float y)
{
    gDrag.active = true;
    gDrag.startX = x;
    gDrag.startY = y;
    gDrag.x = x;
    gDrag.y = y;
}

void pushMouseUp(const float x, const float y)
{
    if (!gDrag.active)
    {
        return; // soltou sem ter apertado aqui (o aperto foi noutra janela)
    }

    if (gDrops.size() < kDropQueueMax)
    {
        gDrops.push_back(Drop{ true, gDrag.startX, gDrag.startY, x, y });
    }
    gDrag = DragState{};
}

void cancelDrag() { gDrag = DragState{}; }

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

float mouseX() { return gMouse.x(); }
float mouseY() { return gMouse.y(); }

MouseClick readMouseClick() { return gMouse.readClick(); }

DragState drag() { return gDrag; }

Drop readDrop()
{
    if (gDrops.empty())
    {
        return Drop{};
    }

    const Drop front = gDrops.front();
    gDrops.erase(gDrops.begin());
    return front;
}

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
