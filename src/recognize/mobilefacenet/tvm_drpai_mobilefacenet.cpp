/*****************************************
* Includes
******************************************/
#include "tvm_drpai_mobilefacenet.h"

TVM_MobileFaceNet_DRPAI::TVM_MobileFaceNet_DRPAI() :
    IRecognizeModel(TVM_MODEL_OUT_C,
        TVM_MODEL_DIR.data(), MODEL_NAME.data(),
        TVM_DRPAI_IN_WIDTH, TVM_DRPAI_IN_HEIGHT, TVM_DRPAI_IN_CHANNEL,
        TVM_MODEL_IN_W, TVM_MODEL_IN_H, TVM_MODEL_IN_C, MODE_TVM_MOBILEFACENET_DRPAI)
{
    int8_t ret = 0;
    constructor_err = 0;

    /*Get DRP-AI memory area start address*/
    errno = 0;
    drpai_fd = open("/dev/drpai0", O_RDWR);
    if (0 > drpai_fd)
    {
        err_str = "[ERROR] Failed to open DRP-AI Driver : errno=" + std::to_string(errno);
        constructor_err = -1;
    }
    errno = 0;
    ret = ioctl(drpai_fd, DRPAI_GET_DRPAI_AREA, &drpai_data0);
    if (-1 == ret && 0 == constructor_err)
    {
        err_str = "[ERROR] Failed to get DRP-AI Memory Area : errno=" + std::to_string(errno);
        constructor_err = -1;
    }
    pre_start_addr += drpai_data0.address;
    if (0 <= drpai_fd)
    {
        errno = 0;
        ret = close(drpai_fd);
        if (0 != ret && 0 == constructor_err)
        {
            err_str = "[ERROR] Failed to close DRP-AI Driver : errno=" + std::to_string(errno);
            constructor_err = -1;
        }
    }

    ret = preruntime.Load(pre_dir, pre_start_addr);
    if (0 != ret && 0 == constructor_err)
    {
        err_str = "[ERROR] Failed to load DRP-AI Pre-processing Runtime Object: " + pre_dir;
        err_str = err_str + ".\nPrepare the Pre-processing Runtime Object for MobileFaceNet (see README).";
    }

    in_param.pre_in_shape_w = TVM_DRPAI_IN_WIDTH;
    in_param.pre_in_shape_h = TVM_DRPAI_IN_HEIGHT;
    in_param.pre_in_format = FORMAT_YUYV_422;
    in_param.pre_out_format = FORMAT_RGB;
    in_param.resize_w = TVM_MODEL_IN_W;
    in_param.resize_h = TVM_MODEL_IN_H;
    in_param.resize_alg = ALG_BILINEAR;

    FaceGallery::instance().load("face_gallery.txt");
}

/**
 * @brief inf_pre_process
 * @details Crops the detected face (made square to avoid distortion) and resizes it.
 */
int32_t TVM_MobileFaceNet_DRPAI::inf_pre_process(uint8_t* input_data, uint32_t width, uint32_t height, uint32_t crop_left, uint32_t crop_top, uint32_t addr, float** arg, uint32_t* buf_size)
{
    int32_t side = (int32_t)std::max(width, height);
    if (side > TVM_DRPAI_IN_HEIGHT) side = TVM_DRPAI_IN_HEIGHT;
    int32_t cx = (int32_t)crop_left + (int32_t)width / 2;
    int32_t cy = (int32_t)crop_top + (int32_t)height / 2;
    int32_t left = cx - side / 2;
    int32_t top = cy - side / 2;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (left > TVM_DRPAI_IN_WIDTH - side) left = TVM_DRPAI_IN_WIDTH - side;
    if (top > TVM_DRPAI_IN_HEIGHT - side) top = TVM_DRPAI_IN_HEIGHT - side;
    /* YUYV packs two pixels per chroma sample, keep the crop even-aligned */
    left &= ~1;
    side &= ~1;

    in_param.crop_w = (uint16_t)side;
    in_param.crop_h = (uint16_t)side;
    in_param.crop_tl_x = (uint16_t)left;
    in_param.crop_tl_y = (uint16_t)top;

    return pre_process_drpai(addr, arg, buf_size);
}

/**
 * @brief inf_post_process
 * @details L2-normalize the embedding and match it with the gallery by cosine similarity.
 */
int32_t TVM_MobileFaceNet_DRPAI::inf_post_process(float* arg)
{
    std::vector<float> emb(arg, arg + TVM_MODEL_OUT_C);
    FaceGallery::normalize(emb);
    matches.push_back(FaceGallery::instance().identify(emb));
    embeddings.push_back(std::move(emb));
    return 0;
}

int32_t TVM_MobileFaceNet_DRPAI::print_result()
{
    for (size_t i = 0; i < matches.size(); i++)
    {
        printf(" Face %zu: %s (cosine %.3f)\n", i + 1, matches[i].name.c_str(), matches[i].similarity);
    }
    return 0;
}

shared_ptr<PredictNotifyBase> TVM_MobileFaceNet_DRPAI::get_command()
{
    ObjectDetection* ret = new ObjectDetection();

    /* The first match is the sole face selected inside the center guide. */
    std::string enroll_name;
    if (!matches.empty() && !embeddings.empty() &&
        FaceGallery::instance().take_pending(enroll_name))
    {
        if (FaceGallery::instance().enroll(enroll_name, embeddings.front()))
        {
            matches.front() = {enroll_name, 1.0f, true};
            ret->message = "Enrolled: " + enroll_name;
        }
        else
        {
            ret->message = "Failed to save the face gallery";
            std::cerr << "[ERROR] Failed to save the face gallery." << std::endl;
        }
    }

    for (size_t i = 0; i < detected_data.size(); i++)
    {
        bbox_t dat;
        const detection& det = detected_data[i];
        dat.X = (int32_t)det.bbox.x;
        dat.Y = (int32_t)det.bbox.y;
        dat.W = (int32_t)det.bbox.w;
        dat.H = (int32_t)det.bbox.h;
        dat.name = i < matches.size() ? matches[i].name : "Unknown";
        dat.pred = i < matches.size()
            ? std::max(0.0f, matches[i].similarity) * 100.0f
            : 0.0f;
        ret->predict.push_back(dat);
    }
    embeddings.clear();
    matches.clear();
    return shared_ptr<PredictNotifyBase>(move(ret));
}

int8_t TVM_MobileFaceNet_DRPAI::pre_process_drpai(uint32_t addr, float** output_buf, uint32_t* buf_size)
{
    in_param.pre_in_addr = (uintptr_t)addr;
    if (0 != preruntime.Pre(&in_param, (void**)output_buf, buf_size))
    {
        std::cerr << "[ERROR] Failed to run DRP-AI Pre-processing Runtime." << std::endl;
        return -1;
    }
    return 0;
}
