#ifndef WIDGET_H
#define WIDGET_H

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
    float boidModelScale;

    SimulationParams(float avoidR = 1.2f, float avoidF = 2.0f,
        float alignR = 1.5f, float alignF = 0.8f,
        float cohesionR = 2.0f, float cohesionF = 0.4f,
        float boundMn = -8.0f, float boundMx = 8.0f,
        float bounceF = 1.5f, int boidNum = 200,
        float dt = 0.05, float scale = 0.015f)
        : avoidForce(avoidF), avoidRadius(avoidR),
        alignForce(alignF), alignRadius(alignR),
        cohesionForce(cohesionF), cohesionRadius(cohesionR),
        boundMin(boundMn), boundMax(boundMx),
        bounceForce(bounceF), boidNumber(boidNum),
        deltaTime(dt), boidModelScale(scale) {
    }
};

#endif //WIDGET_H
