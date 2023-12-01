#define OLC_PGE_APPLICATION
#define OLC_PGEX_FONT
#include "olcPixelGameEngine.h"
#include "titleScreen.h"
#include "shadowCast.h"
#include "3d.h"

using namespace olc;

ShadowCaster sc;
Engine3D e3d;


class Main : public PixelGameEngine
{
    private:
        int mouseX, mouseY;
        int prevMouseX, prevMouseY;
        int screenW, screenH;
        HCURSOR hCursor;

    public:
        Main()
        {
            sAppName = "Test";
            bShowFPS = true;
        }
    
    public:
    bool OnUserCreate() override
    {
        //titleScreen::Create(this);
        //sc.Create(this);
        e3d.Create(this, "City2");

        screenW = ScreenWidth();
        screenH = ScreenHeight();

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        //titleScreen::Update(this);
        //sc.Update();

        mouseX = max(0, min(ScreenWidth(), GetMouseX()));
        mouseY = max(0, min(ScreenHeight(), GetMouseY()));
        
        //Speed
        float moveSpeed = 8;
        if (GetKey(Key::SHIFT).bHeld)
            moveSpeed = 24;
        if (GetKey(Key::CTRL).bHeld)
            moveSpeed = 4;

        moveSpeed *= fElapsedTime;

        //Look mouse
        if (GetMouse(1).bHeld)
        {
            moveSpeed *= 32;
            bMouseWrapToWindow = true;
            float dx = (prevMouseX - mouseX)%(screenW-1) * -moveSpeed * 0.0025f;
            float dy = (prevMouseY - mouseY)%(screenH-1) * moveSpeed * 0.005f;
            e3d.LookRight(dx);
            e3d.LookUp(dy);
            e3d.SetOverrideLookAt(true);
        }
        else
        {
            bMouseWrapToWindow = false;
            e3d.SetOverrideLookAt(false);
        }

        //Move
        if (GetKey(Key::Q).bHeld)
            e3d.MoveUp(-moveSpeed);
        if (GetKey(Key::W).bHeld)
            e3d.MoveForward(moveSpeed);
        if (GetKey(Key::E).bHeld)
            e3d.MoveUp(moveSpeed);
        if (GetKey(Key::A).bHeld)
            e3d.MoveRight(-moveSpeed);
        if (GetKey(Key::S).bHeld)
            e3d.MoveForward(-moveSpeed);
        if (GetKey(Key::D).bHeld)
            e3d.MoveRight(moveSpeed);
        //Look arrows
        if (GetKey(Key::RIGHT).bHeld)
            e3d.LookRight(0.25 * moveSpeed);
        if (GetKey(Key::LEFT).bHeld)
            e3d.LookRight(-0.25 * moveSpeed);
        if (GetKey(Key::UP).bHeld)
            e3d.LookUp(0.25 * moveSpeed);
        if (GetKey(Key::DOWN).bHeld)
            e3d.LookUp(-0.25 * moveSpeed);
        //Actions
        if (GetKey(Key::SPACE).bPressed)
            e3d.NextPathPoint();
        if (GetKey(Key::F3).bPressed)
            e3d.ToggleDebugMode();
        if (GetKey(Key::R).bReleased)
            e3d.ResetPaths();
        if (GetKey(Key::L).bPressed)
            system("start https://gims-cc.co.za");
        float scroll = GetMouseWheel()/50.0f;
        if (scroll != 0.0f)
        {
			if (GetKey(Key::SHIFT).bHeld)
			{
				e3d.ZoomDepth(scroll*10.0f);
			}
			else
			{
				e3d.ZoomFOV(scroll);
			}
        }

        e3d.Update(this, fElapsedTime);

        prevMouseX = mouseX;
        prevMouseY = mouseY;

		return true;
    }

};

int main()
{
    Main main;

    if(main.Construct(512, 512, 2, 2))
    {
        main.Start();
    }

    return 0;
}
