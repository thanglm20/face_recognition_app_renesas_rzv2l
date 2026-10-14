const cameraCanvas = document.getElementById("camera");
const resultCanvas = document.getElementById("result");
const cameraContext = cameraCanvas.getContext("2d");
const resultContext = resultCanvas.getContext("2d");
const endpoint = document.getElementById("endpoint");
const status = document.getElementById("status");
const faces = document.getElementById("faces");
const timing = document.getElementById("timing");
const message = document.getElementById("message");

let socket;
let cameraImage;
let frameWidth = resultCanvas.width;
let frameHeight = resultCanvas.height;

function setStatus(text, state) {
  status.textContent = text;
  status.dataset.state = state;
}

function drawImage(canvasContext, image, boxes) {
  canvasContext.drawImage(image, 0, 0, resultCanvas.width, resultCanvas.height);
  const scaleX = resultCanvas.width / frameWidth;
  const scaleY = resultCanvas.height / frameHeight;
  canvasContext.lineWidth = 3;
  canvasContext.strokeStyle = "#ffe600";
  canvasContext.fillStyle = "#ffe600";
  canvasContext.font = "16px sans-serif";
  boxes.forEach((box) => {
    canvasContext.strokeRect(box.X * scaleX, box.Y * scaleY,
      box.W * scaleX, box.H * scaleY);
    canvasContext.fillText(`${Number(box.pred).toFixed(1)}%`,
      box.X * scaleX, Math.max(16, box.Y * scaleY - 4));
  });
}

function handleMessage(event) {
  let data;
  try {
    data = JSON.parse(event.data);
  } catch (error) {
    message.textContent = "Invalid JSON received from device";
    return;
  }

  if (data.command_name === "camera_image") {
    cameraImage = new Image();
    cameraImage.onload = () => {
      cameraContext.drawImage(cameraImage, 0, 0, cameraCanvas.width, cameraCanvas.height);
    };
    cameraImage.src = `data:image/jpeg;base64,${data.Value.img}`;
    return;
  }

  if (data.command_name === "object_detection") {
    const value = data.Value;
    frameWidth = Number(value.img_org_w) || frameWidth;
    frameHeight = Number(value.img_org_h) || frameHeight;
    const boxes = Array.isArray(value.predict) ? value.predict : [];
    faces.textContent = String(boxes.length);
    timing.textContent = `${Number(value.drp_time).toFixed(2)} ms`;
    const resultImage = new Image();
    resultImage.onload = () => drawImage(resultContext, resultImage, boxes);
    resultImage.src = `data:image/jpeg;base64,${value.img}`;
    return;
  }

  if (data.command_name === "app_message") {
    message.textContent = data.Value.message || "-";
  }
}

document.getElementById("connection-form").addEventListener("submit", (event) => {
  event.preventDefault();
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.close();
  }
  setStatus("Connecting...", "connecting");
  socket = new WebSocket(endpoint.value.trim());
  socket.onopen = () => setStatus("Connected", "connected");
  socket.onmessage = handleMessage;
  socket.onerror = () => {
    setStatus("Connection error", "error");
    message.textContent =
      "Check the board IP, port 3000, Ethernet link, and device server log";
  };
  socket.onclose = (event) => {
    if (event.code === 1000) {
      setStatus("Disconnected", "disconnected");
      return;
    }
    setStatus(`Disconnected (${event.code})`, "error");
    message.textContent = event.reason || "WebSocket handshake/connection failed";
  };
});
