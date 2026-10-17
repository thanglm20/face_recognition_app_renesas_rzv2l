const displayCanvas = document.getElementById("display");
const displayContext = displayCanvas.getContext("2d");
const RESULT_HOLD_MS = 500;
const endpoint = document.getElementById("endpoint");
const status = document.getElementById("status");
const faces = document.getElementById("faces");
const timing = document.getElementById("timing");
const message = document.getElementById("message");
const personName = document.getElementById("person-name");

let socket;
let cameraImage;
let lastResultAt = 0;
let frameWidth = displayCanvas.width;
let frameHeight = displayCanvas.height;
let awaitingEnrollName = "";

function setStatus(text, state) {
  status.textContent = text;
  status.dataset.state = state;
}

function drawImage(canvasContext, image, boxes) {
  canvasContext.drawImage(image, 0, 0, displayCanvas.width, displayCanvas.height);
  const scaleX = displayCanvas.width / frameWidth;
  const scaleY = displayCanvas.height / frameHeight;
  drawCenterGuide(canvasContext);
  canvasContext.lineWidth = 3;
  canvasContext.strokeStyle = "#ffe600";
  canvasContext.fillStyle = "#ffe600";
  canvasContext.font = "16px sans-serif";
  boxes.forEach((box) => {
    const known = box.name && box.name !== "Unknown";
    const color = known ? "#00e676" : "#ffe600";
    canvasContext.strokeStyle = color;
    canvasContext.fillStyle = color;
    canvasContext.strokeRect(box.X * scaleX, box.Y * scaleY,
      box.W * scaleX, box.H * scaleY);
    canvasContext.fillText(`${box.name || "Unknown"} ${Number(box.pred).toFixed(1)}%`,
      box.X * scaleX, Math.max(16, box.Y * scaleY - 4));
  });
}

function drawCenterGuide(canvasContext) {
  const left = canvasContext.canvas.width * 0.25;
  const top = canvasContext.canvas.height * 0.15;
  const width = canvasContext.canvas.width * 0.5;
  const height = canvasContext.canvas.height * 0.7;
  canvasContext.save();
  canvasContext.strokeStyle = "#00d9ff";
  canvasContext.lineWidth = 2;
  canvasContext.setLineDash([10, 7]);
  canvasContext.strokeRect(left, top, width, height);
  canvasContext.restore();
}

function isInCenterGuide(box) {
  const centerX = (Number(box.X) + Number(box.W) / 2) / frameWidth;
  const centerY = (Number(box.Y) + Number(box.H) / 2) / frameHeight;
  return centerX >= 0.25 && centerX <= 0.75 &&
    centerY >= 0.15 && centerY <= 0.85;
}

function sendEnroll(action, name) {
  if (!socket || socket.readyState !== WebSocket.OPEN) {
    message.textContent = "Not connected";
    return false;
  }
  socket.send(JSON.stringify({
    command_name: "face_enroll",
    Value: { action, name },
  }));
  return true;
}

document.getElementById("enroll-btn").addEventListener("click", () => {
  const name = personName.value.trim().replace(/\s+/g, "_");
  if (!name) {
    message.textContent = "Enter a name to enroll";
    return;
  }
  if (sendEnroll("enroll", name)) {
    awaitingEnrollName = name;
    message.textContent = `Enrollment queued for "${name}". Place one face inside the center frame.`;
  }
});
document.getElementById("delete-btn").addEventListener("click", () => {
  const name = personName.value.trim().replace(/\s+/g, "_");
  if (name) sendEnroll("delete", name);
});
document.getElementById("clear-btn").addEventListener("click", () => {
  if (sendEnroll("clear", "")) {
    awaitingEnrollName = "";
    message.textContent = "Clearing face gallery...";
  }
});

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
      // Recognition result has priority while faces are being reported
      if (Date.now() - lastResultAt < RESULT_HOLD_MS) return;
      displayContext.drawImage(cameraImage, 0, 0, displayCanvas.width, displayCanvas.height);
      drawCenterGuide(displayContext);
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
    if (awaitingEnrollName) {
      const centered = boxes.filter(isInCenterGuide);
      if (value.message && value.message.startsWith("Enrolled:")) {
        message.textContent = `Enrolled "${awaitingEnrollName}"`;
        awaitingEnrollName = "";
      } else if (value.message && value.message.startsWith("Failed")) {
        message.textContent = value.message;
        awaitingEnrollName = "";
      } else if (centered.length > 1) {
        message.textContent = "Only one face can be inside the center frame";
      } else if (centered.length === 0) {
        message.textContent = "Move one face inside the center frame to enroll";
      } else {
        message.textContent = `Enrolling "${awaitingEnrollName}"...`;
      }
    } else if (value.message) {
      message.textContent = value.message;
    }
    const resultImage = new Image();
    if (boxes.length > 0) {
      lastResultAt = Date.now();
      resultImage.onload = () => drawImage(displayContext, resultImage, boxes);
      resultImage.src = `data:image/jpeg;base64,${value.img}`;
    }
    return;
  }

  if (data.command_name === "app_message") {
    message.textContent = data.Value.message || "-";
    if (/^Could not|^Invalid/.test(message.textContent)) {
      awaitingEnrollName = "";
    }
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
