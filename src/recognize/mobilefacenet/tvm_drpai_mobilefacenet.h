#pragma once

#ifndef DRP_TVM_MODEL_MOBILEFACENET_H
#define DRP_TVM_MODEL_MOBILEFACENET_H

/*****************************************
* Includes
******************************************/
#include "../irecognize_model.h"
#include "../../includes.h"
#include "../common/functions.h"
#include "../command/object_detection.h"
#include "PreRuntime.h"
#include "face_gallery.h"
#include <linux/drpai.h>

#include "../../app_config.h"

class TVM_MobileFaceNet_DRPAI : public IRecognizeModel
{
private:
    constexpr static string_view TVM_MODEL_DIR = "mobilefacenet_onnx";
    constexpr static int32_t TVM_DRPAI_IN_WIDTH = (app_config::CAMERA_WIDTH);
    constexpr static int32_t TVM_DRPAI_IN_HEIGHT = (app_config::CAMERA_HEIGHT);
    constexpr static int32_t TVM_DRPAI_IN_CHANNEL = (2);

    constexpr static string_view MODEL_NAME = "DRP-AI TVM MobileFaceNet (DRP-AI)";
    /*DRP-AI Input image information*/
    constexpr static int32_t TVM_MODEL_IN_C = (3);
    constexpr static int32_t TVM_MODEL_IN_W = (112);
    constexpr static int32_t TVM_MODEL_IN_H = (112);
    constexpr static int32_t TVM_MODEL_OUT_C = (app_config::FACE_EMBEDDING_DIM);
    /*DRP-AI Pre-processing Runtime start address offset*/
    constexpr static int32_t PRE_START_ADDR_OFFSET = (0x4F8200);

public:
    TVM_MobileFaceNet_DRPAI();
    virtual int32_t inf_pre_process
        (uint8_t* input_data, uint32_t width, uint32_t height, uint32_t crop_left, uint32_t crop_top,  uint32_t addr, float** arg, uint32_t* buf_size);
    virtual int32_t inf_post_process(float* arg);
    virtual shared_ptr<PredictNotifyBase> get_command();
    virtual int32_t print_result();

private:
    int8_t pre_process_drpai(uint32_t addr, float** output_buf, uint32_t* buf_size);

private:
    uint32_t     pre_start_addr = PRE_START_ADDR_OFFSET;
    int          drpai_fd = -1;
    drpai_data_t drpai_data0;

    PreRuntime preruntime;
    s_preproc_param_t in_param;
    const std::string pre_dir = std::string(TVM_MODEL_DIR.data()) + "/preprocess";

    /* Per-frame results, aligned with detected_data */
    std::vector<std::vector<float>> embeddings;
    std::vector<FaceGallery::Match> matches;
};
#endif //DRP_TVM_MODEL_MOBILEFACENET_H
