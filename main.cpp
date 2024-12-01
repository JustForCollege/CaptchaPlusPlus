#include "raylib.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "json.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>

#include <ctime>

using json = nlohmann::json;

int ScreenWidth = 450; 
int ScreenHeight = 500; 

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

struct CaptchaCell 
{
    Rectangle bounds;
    bool checked;
};

struct Captcha 
{
    Texture2D texture;
    int checkedCount;
    std::string question; 
    std::vector<int> solution;
    CaptchaCell cells[9];
};

struct Scene 
{
    std::vector<Captcha> captchas;
    int solved; 
    int captchaIndex;
};

Captcha CreateCaptcha(const char* imagePath, std::string question, std::vector<int> solution)
{
    Image image = LoadImage(imagePath);
    Texture2D texture = LoadTextureFromImage(image); 
    UnloadImage(image);
    
    Captcha captcha
    {
        .texture = texture,
        .question = question,
        .solution = solution,
    };
    
    int cellWidth = texture.width / 3; 
    int cellHeight = texture.height / 3; 
    
    for(int row = 0; row < 3; row++)
    {
        for(int col = 0; col < 3; col++)
        {
           Rectangle source
           {
               .x = (float)col * cellWidth, 
               .y = (float)row * cellHeight, 
               .width = (float)cellWidth, 
               .height = (float)cellHeight,
           };
            
            captcha.cells[col + row * 3] = {
                .bounds = source, 
                .checked = false, 
            };
        }
    }
    
    return captcha;
}

Vector2 GetCaptchaCellPos(const Captcha& captcha, const CaptchaCell& cell, int row, int col)
{
    constexpr int spacing = 10;   
    int offsetX = ((ScreenWidth / 2) - (captcha.texture.width / 2)) - spacing;
    int offsetY = ((ScreenHeight / 2) - (captcha.texture.height / 2)) - spacing;
    
    Vector2 pos{};
    
    if(cell.checked)
    {
        pos = { 5 + offsetX + cell.bounds.x + col*spacing, 5 + offsetY + cell.bounds.y + row*spacing};
    }
    else 
    {
        pos = { offsetX + cell.bounds.x + col*spacing, offsetY + cell.bounds.y + row*spacing};
    }
    
    return pos;
}

void DrawCaptcha(const Captcha& captcha)
{
    for(int row = 0; row < 3; row++)
    {
        for(int col = 0; col < 3; col++)
        {
            CaptchaCell cell = captcha.cells[col + row * 3];
            Vector2 pos = GetCaptchaCellPos(captcha, cell, row, col);
            
            
            if(cell.checked)
            {
                Rectangle src = { cell.bounds.x, cell.bounds.y, cell.bounds.width, cell.bounds.height };
                Rectangle dst = { pos.x, pos.y, cell.bounds.width - 10, cell.bounds.height - 10 };
                
                DrawTexturePro(captcha.texture, src, dst, {}, 0, WHITE);
                DrawRectangleLinesEx({ pos.x, pos.y, cell.bounds.width - 10, cell.bounds.height - 10 }, 2, GREEN);
            }
            else 
            {
                DrawTextureRec(captcha.texture, cell.bounds, pos, WHITE);      
            }
        }
    }
}

void CaptchaHandleCheck(Captcha& captcha)
{
    if(!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
    
    Vector2 mousePos = GetMousePosition();
    
    for(int row = 0; row < 3; row++)
    {
        for(int col = 0; col < 3; col++)
        {
            auto& cell = captcha.cells[col + row * 3];            
            Vector2 pos = GetCaptchaCellPos(captcha, cell, row, col);
            
            
            if(CheckCollisionPointRec(mousePos, { pos.x, pos.y, cell.bounds.width, cell.bounds.height } ))
            {
                cell.checked = !cell.checked;
                
                if(cell.checked)
                { 
                    captcha.checkedCount++;
                }
                else
                {  
                  captcha.checkedCount =  MAX(captcha.checkedCount - 1, 0);
                }
            }
        }
    }
}

struct Button 
{
    int x; 
    int y; 
    int width; 
    int height; 
    const char* text;
    bool hovered;
    int fontSize;
    Color textColor;
    Color color; 
};

Button CreateButtonWithText(const char* text, int fontSize, int x, int y, int width, int height, Color color, Color textColor)
{
    Button button{};
    
    button.x = x; 
    button.y = y; 

    button.width = width;
    button.height = height;
    button.color = color;
    button.fontSize = fontSize; 
    button.color = color; 
    button.text = text; 
    button.textColor = textColor;
    
    return button;
}

void DrawButton(const Button& button)
{
    if(!button.hovered)
    {
        DrawRectangle(button.x, button.y, button.width, button.height, button.color);
    }
    else
    {
        Color buttonColor = button.color;
        
        unsigned char r = MIN( (uint8_t) ( buttonColor.r * 1.9 ), 255 );
        unsigned char g = MIN( (uint8_t) ( buttonColor.g * 1.9 ), 255 );
        unsigned char b = MIN( (uint8_t) ( buttonColor.b * 1.9 ), 255 );
        
        Color newColor = { r, g, b, buttonColor.a };
        DrawRectangle(button.x, button.y, button.width, button.height, newColor);
    }
    
    if(button.text)
    {
        int textWidth = MeasureText(button.text, button.fontSize); 
        int textHeight = GetFontDefault().baseSize;     
        float textX = button.x + (button.width / 2 -  textWidth / 2);
        float textY = button.y + (button.height / 2  - textHeight / 2);
        DrawText(button.text, textX, textY, button.fontSize, button.textColor);
    }
}

void HandleButtonHovered(Button& button)
{
    Vector2 mousePos = GetMousePosition();
    
    Rectangle buttonRec = { (float)button.x, (float)button.y, (float)button.width, (float)button.height };
    
    if(CheckCollisionPointRec(mousePos, buttonRec))
        button.hovered = true;
    else 
        button.hovered = false; 
}

bool IsButtonPressed(const Button& button)
{
    if(!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return false; 
    
    Vector2 mousePos = GetMousePosition();
    Rectangle buttonRec = { (float)button.x, (float)button.y, (float)button.width, (float)button.height };
    
    return CheckCollisionPointRec(mousePos, buttonRec);
}

auto LoadCaptchasMeta(const std::string& filePath) -> std::vector<json>
{
    std::ifstream f(filePath);
    json meta = json::parse(f);
    
    return meta["captchas"].get<std::vector<json>>();
}

bool VerifyCaptcha(const Captcha& captcha)
{
    if(captcha.checkedCount != (int)captcha.solution.size()) return false; 
    
    for(const auto& cell : captcha.solution)
    {
       if(!captcha.cells[cell - 1].checked)
            return false;
    }
    
    return true;
}

Scene CreateScene()
{
    Scene scene{};
    
    auto captchasMeta = LoadCaptchasMeta("data/meta.json");

    for(const auto& captchaMeta: captchasMeta)
    {
        std::string imagePath = captchaMeta["image_path"];
        std::string question = captchaMeta["question"];
        std::vector<int> solution = captchaMeta["solution"];
        
        Captcha captcha = CreateCaptcha(imagePath.c_str(), question, solution);
        
        scene.captchas.push_back(captcha);
    }
    
    return scene;
}

void UpdateScene(Scene& scene)
{
    scene.captchaIndex++;
    scene.solved++;
    
    if(scene.captchaIndex >= (int)scene.captchas.size())
    {
        scene.captchaIndex = 0; 
        scene.solved = 0;
        
        for(auto& captcha: scene.captchas)
        {
            captcha.checkedCount = 0;
            
            for(auto& cell: captcha.cells)
                cell.checked = false; 
        }
    }
}

int main()
{
    #ifdef __EMSCRIPTEN__
       ScreenWidth = GetMonitorWidth(GetCurrentMonitor());
       ScreenHeight = GetMonitorHeight(GetCurrentMonitor());
    #else 
       ScreenWidth = 450; 
       ScreenHeight = 500;
    #endif 
    
    InitWindow(ScreenWidth, ScreenHeight, "Captcha");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);
    
    Scene scene = CreateScene();
    
    int buttonWidth = 110;
    int buttonHeight = 50; 
    int buttonX = ScreenWidth / 2 - buttonWidth / 2;
    int buttonY = ScreenHeight - buttonHeight;
    Button verifyButton = CreateButtonWithText("Verify", 20, buttonX, buttonY, buttonWidth, buttonHeight, GREEN, BLACK);
    
    Color questionColor = WHITE;
       
    while(!WindowShouldClose())
    {
        auto& currentCaptcha = scene.captchas[scene.captchaIndex];
        auto currentQuestion = currentCaptcha.question;
        
        BeginDrawing();
        ClearBackground(BLACK);
        
        CaptchaHandleCheck(currentCaptcha);
        HandleButtonHovered(verifyButton);
        
        DrawCaptcha(currentCaptcha);
        DrawButton(verifyButton);
        
        
        if(IsButtonPressed(verifyButton))
        {
            if(VerifyCaptcha(scene.captchas[scene.captchaIndex]))
            {
                questionColor = GREEN;
                UpdateScene(scene);
                questionColor = WHITE;
            }
            else 
            {
                questionColor = RED;
            }
        }
        
        const char* scoreText = TextFormat("%d / %d", scene.solved, scene.captchas.size());
        
        DrawText(scoreText, ScreenWidth - MeasureText(scoreText, 20) - 10, ScreenHeight - 20, 20, WHITE);
        DrawText(currentQuestion.c_str(), ScreenWidth / 2 - MeasureText(currentQuestion.c_str(), 30) / 2, 30, 30, questionColor);
        
        EndDrawing();
    }
}