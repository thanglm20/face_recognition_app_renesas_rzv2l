# Face Detection and Recognition Application

This is a standalone device application extracted from the USB camera sample.
It is outside `how-to/sample_app` so its source and build configuration can
evolve independently.

## Pipeline

```text
USB camera (YUYV) -> V4L2 + u-dma-buf
        |
        v
DRP-AI TVM UltraFace  -> face boxes
        |  (square crop of each face, resized to 112x112 RGB by DRP-AI pre-processing)
        v
DRP-AI TVM MobileFaceNet -> embedding (L2-normalized)
        |
        v
Cosine similarity vs. enrolled gallery -> name / Unknown
        |
        v
WebSocket server -> PC browser
```

Every detected face is shown with a bounding box in the WebSocket client.
Recognition labels and cosine similarity (in percent) are overlaid on the
annotated inference image. Matches below the threshold (`0.5`, set in
`FaceGallery::threshold`) are shown as `Unknown`. Recognition runs only when
exactly one detected face center is inside the center guide (the central 50%
of the image width and 70% of its height). The original camera JPEG and the
annotated inference JPEG are both sent to `etc/Face_Recognition_Client`.

### Enrollment

In the browser client type a name and press **Enroll face in center frame**.
The embedding from the next frame containing exactly one face in the guide is
saved with that name. Several enrollments with the same name improve accuracy.
**Delete person** and **Clear all** remove entries. The gallery is stored in
`face_gallery.txt` in the working directory of the application (one line per
sample: `name v0 v1 ...`) and loaded at start-up. Updates are written through
a temporary file so an unsuccessful write does not discard the previous
gallery.

## Source layout

| Path | Purpose |
| --- | --- |
| `src/camera` | V4L2 USB camera and u-dma-buf capture |
| `src/recognize/ultraface` | UltraFace DRP-AI preprocessing/inference/postprocessing |
| `src/recognize/mobilefacenet` | MobileFaceNet embedding model and cosine-similarity gallery |
| `src/recognize` | Inference pipeline and model interface |
| `src/ws_server.*` | WebSocket server |
| `src/command` | JSON messages (results to the browser, `face_enroll` from the browser) |
| `src/image_converter.*` | YUYV to JPEG conversion |
| `etc/Face_Recognition_Client` | PC browser client |
| `models` | Place model objects here when packaging the device image |


## Convert Mobilefacenet

https://www.amebaiot.com/en/amebapro2-mobilefacenet-convert-to-onnx/
https://github.com/foamliu/MobileFaceNet
```
sed -i 's/config.shape_in     = \[1, 480, 640, 3\]/config.shape_in     = [1, 480, 640, 2]/' compile_onnx_model.py
sed -i 's/config.format_in    = drpai_param.FORMAT.BGR/config.format_in    = drpai_param.FORMAT.YUYV_422/' compile_onnx_model.py

sed -i 's/mean    = \[0.485, 0.456, 0.406\]/mean         = [127.5, 127.5, 127.5]/' compile_onnx_model.py
sed -i 's/stdev   = \[0.229, 0.224, 0.225\]/scale_factor = [1.0\/127.5, 1.0\/127.5, 1.0\/127.5]/' compile_onnx_model.py
sed -i '/cof_add =/ c\    cof_add = [-m for m in mean]' compile_onnx_model.py
sed -i '/cof_mul =/ c\    cof_mul = scale_factor' compile_onnx_model.py

# Thêm Crop (app crop từng khuôn mặt lúc chạy)
sed -i 's/    config.ops = \[/    config.ops = [\n        op.Crop(0, 0, config.shape_in[2], config.shape_in[1]),/' compile_onnx_model.py
```
```sh
python3 compile_onnx_model.py \
-i input0 \
-s 1,3,112,112 \
-o mobilefacenet_onnx \
mobilefacenet.onnx
```

The application expects directory `mobilefacenet_onnx` with 112x112 RGB input
(YUYV 640x480 -> crop/resize -> RGB, same pre-processing as the UltraFace
recipe but with RGB output and the normalization your model was trained with,
typically `(x - 127.5) / 128`) and an embedding output of 128 values. For a
different embedding size (e.g. 512) configure with
`-DCMAKE_CXX_FLAGS=-DMOBILEFACENET_EMB_DIM=512`.

Camera resolution, detection thresholds, the center guide area, the embedding
size and the match threshold are all defined in `src/app_config.h`.

The `preprocess` directory must take the YUYV camera frame and contain a Crop
operator. If the device logs `Parameter change is not allowed if In=Not YUV &
New In=YUV`, the directory was compiled with RGB input; regenerate it with
`scripts/compile_mobilefacenet_preprocess.py` (run inside the DRP-AI TVM
docker, from the `tutorials` directory, with `PRODUCT` set, e.g. `V2L`) and
copy the new `preprocess` directory to the device.

## Build

The build must run in the Renesas SDK/cross-compile environment with OpenCV,
TurboJPEG, DRP-AI TVM runtime, and the DRP-AI headers available.

```sh
cd /path/to/drp-ai_tvm/face_recognition_app
bash scripts/build.sh

or

cmake -DCMAKE_TOOLCHAIN_FILE=$TVM_ROOT/apps/toolchain/runtime.cmake ..
make 
```

```sh
JOBS=4 bash scripts/build.sh
```

The build output is:

```text
build/face_recognition_drpai_tvm_ws
```

## Device files

Copy these files/directories to the device:

```text
exe/face_recognition_drpai_tvm_ws
ultraface_onnx/
mobilefacenet_onnx/
```

`ultraface_onnx` and `mobilefacenet_onnx` must each contain `deploy.json`, `deploy.params`,
`deploy.so`, and the generated `preprocess` directory. The matching
`libtvm_runtime.so` must be available under `/usr/lib64`.

Run the server:

```sh
./face_recognition_drpai_tvm_ws
```

The server listens on `0.0.0.0:3000` by default. A specific bind address can
be passed as the first argument:

```sh
./face_recognition_drpai_tvm_ws 192.168.1.11
```

Open `etc/Face_Recognition_Client/index.html` on the PC and set the endpoint
to `ws://<BOARD_IP>:3000/ws/`.
