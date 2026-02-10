# One-Pass-Boundary-Particle-Driven-Gaussian-Splatting-for-Unified-Physics-and-Rendering

## Overview

본 연구는 실시간 PBD(Position-Based Dynamics) 변형체 시뮬레이션과 3D Gaussian Splatting(3DGS) 기반 고품질 렌더링을 하나의 일관된 표현(Particle–Gaussian) 으로 통합하는 one-pass 프레임워크를 제안한다.

---

## Problem Statement & Motivation

### Why this matters

3D Gaussian Splatting(3DGS)은 정적 장면에서 고품질 + 실시간 뷰 합성을 보여줬지만, 대부분의 방법은 다수의 입력 영상 기반 사전 최적화를 전제로 한다.
반면 실시간 물리 시뮬레이션은 매 프레임 기하가 바뀌고, 특히 cutting / tearing / fracture 같은 topology change가 빈번하다.

### The gap

이 둘을 그대로 결합하면 문제가 생긴다.

- 영상 기반 3DGS는 정적/준정적 가정이라, 시뮬레이션 결과를 즉시 반영하기 어렵다.

- topology change 이후에는 연결성/표면이 급변해, 재최적화 없이는 안정적인 표현이 힘들다.

### Problem statement

> 입력 영상이나 학습/최적화에 의존하지 않고,
> topology change를 포함한 실시간 PBD 시뮬레이션 결과를
> Gaussian primitive로 안정적이고 일관되게(one-pass) 렌더링할 수 있는가?

### Key motivation

이 문제를 풀려면 “영상”과 “메시” 사이에서, 물리 엔진이 제공하는 정보를 그대로 활용하는 중간 표현이 필요합니다.
본 프로젝트는 그 해법으로 **Boundary Particle**을 사용해, topology change 중에도 경계의 밀도/기하 정보를 안정적으로 유지하고 이를 **Gaussian 분포의 기준** 으로 삼는다.

--- 

## Key Ideas / Constributions 

- Boundary Particles for robust topology changes
절단/찢어짐 등 topology change 상황에서도 경계 영역의 입자 밀도와 기하 정보를 안정적으로 유지해 표면 표현의 일관성을 확보.

- Training-free particle → Gaussian mapping
시뮬레이션에서 생성·갱신되는 입자를 추가 학습/최적화 없이 즉시 Gaussian primitive로 변환하여 물리 결과를 곧바로 렌더링에 반영.

- Temporally stable representation (isotropic + position-based appearance)
view-direction 의존성과 불연속을 줄이기 위해 isotropic Gaussian과 position-based appearance를 사용해 시간적 일관성(temporal coherence) 을 강화.

- One-pass unified pipeline (no mesh reconstruction / post-processing)
시뮬레이션과 렌더링이 동일한 particle–Gaussian 표현을 공유하여 메시 재구성이나 후처리 없이 변형·절단·찢어짐 장면을 one-pass로 실시간 처리.

---

## Pipeline / System overview

One-pass loop

- Update: ApplyExtForces → (ProjectDistance + SolveOverpressure) × N → UpdateParticles

- Render: Particle → Gaussian → 3DGS Splatting

<img width="4924" height="1124" alt="image" src="https://github.com/user-attachments/assets/a43f593e-40f9-462a-b901-27895fa5247e" />

### Core Modules (detailed)

1) Distance Constraints (shape preservation)

- 역할: stretch/structural 유지, 기본적인 변형 안정화

- 포인트: 반복(iterations)로 수렴, topology change 이후에도 국소 안정성 유지

- Implementation: [상세 보기](https://github.com/hyeonnmin/Position-Based-Dynamics-in-Plane-Cloth)

2) Overpressure / Volume Constraints (balloon-like behavior)

- 역할: 목표 부피 유지/증가로 풍선 효과, 내부 압력 기반 복원력

- 포인트: 큰 변형에서도 형태 유지, constraint와 함께 반복 수렴

- Implementation: [상세 보기](https://github.com/hyeonnmin/Position-Based-Dynamics-in-ClothBalloons)

3) Boundary Particles (topology change robustness)

- 역할: cutting/tearing 이후 경계의 샘플 밀도/기하를 보강해 surface coherence 유지

- 포인트: 새 경계가 생겨도 Gaussian 분포가 얇아지거나 깨지지 않도록 보조 표현 제공

- Implementation: [상세 보기](https://github.com/hyeonnmin/PBD-ClothBalloon-BoundaryParticles)


