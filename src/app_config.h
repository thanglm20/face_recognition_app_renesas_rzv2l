#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * Central configuration of the face recognition application.
 * The camera resolution must match the `shape_in` used when the DRP-AI
 * pre-processing of ultraface_onnx and mobilefacenet_onnx was compiled
 * (YUYV_422, [1, CAMERA_HEIGHT, CAMERA_WIDTH, 2]).
 */
/* Recognition feature switches (macros: checked with #ifdef) */
#define JPEG_QUALUTY (100)           /* JPEG 0 - 100 */
#define INFERENE_ON                 /* Enable inference.*/
#define SEND_INFERENCE_RESULT_ON    /* Enable send inference image*/
#define SEND_CAMERA_ON              /* Enable send through image */
// #define COUT_INFERENCE_RESULT_ON    /* Enable print infference result to console */
#define MODEL_VGA                   /* ON=VGA Capture & Inference. OFF=FHD Capture & Inference */
//#define SEQUENCTCIAL                /* Enable sync capture and inference*/

namespace app_config
{
    /* Camera capture / DRP-AI input resolution */
    constexpr int CAMERA_WIDTH  = 640;
    constexpr int CAMERA_HEIGHT = 480;

    /* UltraFace detection thresholds */
    constexpr float FACE_DETECT_SCORE_TH = 0.7f;
    constexpr float FACE_DETECT_NMS_TH   = 0.5f;

    /* Center guide: only one face whose center lies inside is recognized (ratios of frame size) */
    constexpr float GUIDE_LEFT   = 0.25f;
    constexpr float GUIDE_RIGHT  = 0.75f;
    constexpr float GUIDE_TOP    = 0.15f;
    constexpr float GUIDE_BOTTOM = 0.85f;

    /* MobileFaceNet embedding length (override with -DMOBILEFACENET_EMB_DIM=512) */
#ifdef MOBILEFACENET_EMB_DIM
    constexpr int FACE_EMBEDDING_DIM = MOBILEFACENET_EMB_DIM;
#else
    constexpr int FACE_EMBEDDING_DIM = 128;
#endif

    /* Cosine similarity needed to accept a face as an enrolled person */
    constexpr float FACE_MATCH_THRESHOLD = 0.5f;
}

#endif /* APP_CONFIG_H */
