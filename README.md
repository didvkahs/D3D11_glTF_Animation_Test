# DirectX_GLTF_Viewer

---

이 프로젝트는 20062020year 제작 vigilante-deku GLTF 모델을 DirectX11로 로드하고 렌더링하는 예제입니다.  GLTF parser, DirectX 로더, 렌더링 파이프라인 초기화 및 실행까지 포함된 구조입니다.

---

## [ 프로젝트 구조 ]

```cpp
├── main.cpp            # 프로그램 진입점, 윈도우 생성 및 메인 루프
├── Core.h / Core.cpp   # DirectX 장치 초기화, 리소스 로드, 렌더링, 윈도우 리사이즈 처리
├── ILoader.h / ILoader.cpp
│                       # 모델 로더 인터페이스, 기본 Mesh/Index/Transform 접근자
├── Loader_gltf.h / Loader_gltf.cpp
│                       # GLTF 모델 파서 + DirectX용 데이터 변환
├── Parser_gltf.h / Parser_gltf.cpp
│                       # GLTF(.gltf, .bin) JSON 파싱, 버퍼/버텍스/텍스처 데이터 추출
├── Shader.fx           # HLSL 셰이더 (VS/PS, Texture2DArray 샘플링)
└── Utile.h             # 유틸리티 정의 포함
```

---

## [ 데이터 흐름 ]

1. main.cpp
    - Win32 윈도우 생성
    - Core 객체 생성 후 InitDevice() 호출
    - 메인 루프에서 RenderFrame() 호출
2. Core
    - DirectX11 디바이스, 스왑체인, 렌더타겟, 뷰포트 설정
    - HLSL셰이더 컴파일 및 InputLayout 생성
    - Loader_gltf를 이용해 모델 로드
    - 로드된 버텍스/인덱스/텍스처 데이터를 GPU 버퍼로 업로드
    - 매 프로임 RenderFrame()에서 메쉬 그리기
3. Loader_gltf
    - Parser_gltf를 호출하여 GLTF 파일을 JSON 및 BIN 파싱
    - 버텍스, 인덱스, UV, 노멀, 텍스처 파일 경로, 샘플러 정보 수집
    - DirectX 구조 ( DX_Vertex_s, DX_Textrue_S ) 로 변환
4. Parasr_gltf
    - nlohmann::jon 사용해 .gltf 파일 파싱
    - bufferView, Accessor, Mesh, Node, Material, Texture 정보 추출
    - 바이너리 버퍼를 읽어 실제 좌표 / 인덱스 / UV 데이터 생성
5. Shader.fx
    - VS : 월드 / 뷰 / 프로젝션 변환, 노멀 변환, UV 전달
    - PS : Texture2DArray 에서 해당 메쉬 인덱스의 텍스처 샘플링

---

## [ 사용 라이브러리 및 라이선스 ]

- 이 프로젝트는 다음 오픈소스 라이브러리를 사용합니다.
- 각 라이브러리의 라이선스 조건을 준수해야 하며, 상세한 라이선스 전문은 해당 프로젝트 저장소를 참고하시기 바랍니다.

1. EASTL
- 저작권자 : Electronic Arts Inc.
- 라이선스 : BSD License
- 용도 : 표준 C++ STL 대체 컨테이너 사용
- 원본 : https://github.com/electronicarts/EASTL

1. DirectXTex
- 저작권자 : Microsoft Corporation
- 라이선스 : MIT License
- 용도 : 텍스처 로드, 변환, 리사이징
- 원본 : https://github.com/microsoft/DirectXTex

1. nlohamnn::json
- 저작권자 : Niel Lohmann
- 라이선서 : MIT Licence
- 용도 : GLTF 파일 파싱
- 원본 : https://github.com/nlohmann/json

1. Vigilante-Deku
- 저작권자 : 20062020Year ( sketchfab )
- 라이선스 : Sketchgab 제공 라이선스 조건에 따름
- 원본 : [Vigilante-deku - Download Free 3D model by 20062020year (@20062020year) [b0d5022]](https://sketchfab.com/3d-models/vigilante-deku-b0d502200ee64e41bdc37cd59cec0e5f)
