#ifndef WIDGET_H
#define WIDGET_H

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

struct SimulationParams {
    float avoidRadius;
    float avoidForce;
    float alignRadius;
    float alignForce;
    float cohesionRadius;
    float cohesionForce;
    float deltaTime;
    int boidNumber;
    float boundMin;
    float boundMax;
    float bounceForce;

    SimulationParams(float avoidR = 1.2f, float avoidF = 2.0f,
        float alignR = 1.5f, float alignF = 0.8f,
        float cohesionR = 2.0f, float cohesionF = 0.4f,
        float boundMn = -8.0f, float boundMx = 8.0f,
        float bounceF = 1.5f, int boidNum = 200,
        float dt = 0.05)
        : avoidForce(avoidF), avoidRadius(avoidR),
        alignForce(alignF), alignRadius(alignR),
        cohesionForce(cohesionF), cohesionRadius(cohesionR),
        boundMin(boundMn), boundMax(boundMx),
        bounceForce(bounceF), boidNumber(boidNum),
        deltaTime(dt) {
    }
};

void initWidget(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");
}

void drawSliderWidget(SimulationParams* params) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Boid Parameters");

    ImGui::SliderFloat("Avoid Force", &params->avoidForce, 0.1f, 5.0f);
    ImGui::SliderFloat("Avoid Radius", &params->avoidRadius, 0.1f, 5.0f);
    ImGui::SliderFloat("Align Force", &params->alignForce, 0.1f, 5.0f);
    ImGui::SliderFloat("Align Radius", &params->alignRadius, 0.1f, 5.0f);
    ImGui::SliderFloat("Cohesion Force", &params->cohesionForce, 0.1f, 5.0f);
    ImGui::SliderFloat("Cohesion Radius", &params->cohesionRadius, 0.1f, 5.0f);
    ImGui::SliderFloat("Delta Time", &params->deltaTime, 0.0f, 0.1f);

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void destroyWidget() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

#endif //WIDGET_H
