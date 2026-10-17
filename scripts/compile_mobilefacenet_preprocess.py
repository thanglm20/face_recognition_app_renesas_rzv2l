"""Regenerate only the DRP-AI pre-processing of MobileFaceNet.

The application feeds a YUYV 640x480 camera frame and crops each detected
face at run time, so the pre-processing must accept YUYV input and contain a
Crop operator. Run it inside the DRP-AI TVM docker, from the `tutorials`
directory (where `drpai_preprocess` lives):

    export PRODUCT=V2L
    python3 /path/to/face_recognition_app/scripts/compile_mobilefacenet_preprocess.py \
        /path/to/face_recognition_app/models/mobilefacenet_onnx

The model files (deploy.json/params/so) are left untouched.
"""
import os
import sys

from drpai_preprocess import *

PRODUCT = os.getenv("PRODUCT")
if PRODUCT is None:
    sys.exit("[Error] Set the PRODUCT environment variable, e.g. export PRODUCT=V2L")

if len(sys.argv) != 2:
    sys.exit("Usage: compile_mobilefacenet_preprocess.py <mobilefacenet_onnx dir>")

output_dir = sys.argv[1]
model_shape_in = [1, 3, 112, 112]

config = preruntime.Config()
config.shape_in = [1, 480, 640, 2]
config.format_in = drpai_param.FORMAT.YUYV_422
config.order_in = drpai_param.ORDER.HWC
config.type_in = drpai_param.TYPE.UINT8

config.shape_out = model_shape_in
config.format_out = drpai_param.FORMAT.RGB
config.order_out = drpai_param.ORDER.CHW
config.type_out = drpai_param.TYPE.FP32

# Same normalization as scripts/onnx_mobilefacenet.py: (x / 255 - 0.5) / 0.5
mean = [127.5, 127.5, 127.5]
cof_add = [-m for m in mean]
cof_mul = [1.0 / 127.5] * 3
config.ops = [
    # Crop starts at maximum size so enough memory is allocated for run-time crops.
    op.Crop(0, 0, config.shape_in[2], config.shape_in[1]),
    op.Resize(model_shape_in[3], model_shape_in[2], op.Resize.BILINEAR),
    op.Normalize(cof_add, cof_mul),
]

preruntime.PreRuntime(config, os.path.join(output_dir, "preprocess"), PRODUCT)
print("Pre-processing written to", os.path.join(output_dir, "preprocess"))
