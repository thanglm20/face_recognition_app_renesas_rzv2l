# Face Detection and Recognition Application

This is a standalone device application extracted from the USB camera sample.
It is outside `how-to/sample_app` so its source and build configuration can
evolve independently.

## Current pipeline

```text
USB camera (YUYV)
        |
        v
V4L2 + u-dma-buf
        |
        v
DRP-AI TVM UltraFace
        |
        v
Face bounding boxes + confidence
        |
        v
WebSocket server -> PC browser
```

The application currently performs **face detection** with UltraFace. It sends
both the original camera JPEG and the inference JPEG with bounding boxes to the
browser client in `etc/Face_Recognition_Client`.

Identity face recognition is a separate stage. To add it, keep UltraFace as
the detector and add a face-embedding model for each detected crop, followed
by a gallery/threshold comparison. The current project retains the
`RecognizeBase` and model interfaces so that stage can be added without
changing the camera or WebSocket transport.

## Source layout

| Path | Purpose |
| --- | --- |
| `src/camera` | V4L2 USB camera and u-dma-buf capture |
| `src/recognize/ultraface` | UltraFace DRP-AI preprocessing/inference/postprocessing |
| `src/recognize` | Inference pipeline and model interface |
| `src/ws_server.*` | WebSocket server |
| `src/command` | JSON messages sent to the browser |
| `src/image_converter.*` | YUYV to JPEG conversion |
| `etc/Face_Recognition_Client` | PC browser client |
| `models` | Place model objects here when packaging the device image |

## Build

The build must run in the Renesas SDK/cross-compile environment with OpenCV,
TurboJPEG, DRP-AI TVM runtime, and the DRP-AI headers available.

```sh
export TVM_HOME=/path/to/drp-ai_tvm/tvm
cd /path/to/drp-ai_tvm/face_recognition_app
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$TVM_HOME/../apps/toolchain/runtime.cmake ..
make -j$(nproc)
```

The output is:

```text
face_recognition_drpai_tvm_ws
```

## Device files

Copy these files/directories to the device:

```text
face_recognition_drpai_tvm_ws
ultraface_onnx/
```

`ultraface_onnx` must contain `deploy.json`, `deploy.params`,
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

