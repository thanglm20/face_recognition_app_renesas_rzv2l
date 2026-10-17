import cv2
import numpy as np
import onnxruntime as ort

def preprocess_image(image_path):
    """
    Tiền xử lý ảnh chuẩn theo repo foamliu/MobileFaceNet:
    - Đọc ảnh, chuyển từ BGR sang RGB.
    - Resize về kích thước chuẩn: 112x112.
    - Chia 255.0 để đưa giá trị pixel về [0.0, 1.0].
    - Chuẩn hóa theo công thức (x - 0.5) / 0.5 để đưa về khoảng [-1.0, 1.0].
    - Chuyển đổi định dạng từ HWC sang CHW.
    - Thêm chiều Batch -> Shape: [1, 3, 112, 112].
    """
    img = cv2.imread(image_path)
    if img is None:
        raise ValueError(f"Không thể đọc file ảnh từ đường dẫn: {image_path}")
    
    # 1. Chuyển BGR sang RGB và resize về 112x112
    # img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = cv2.resize(img, (112, 112))
    
    # Thử chuẩn hóa về [-1, 1] (kiểu PyTorch / MobileFaceNet thông thường)
    img = img.astype(np.float32) / 255.0
    img = (img - 0.5) / 0.5
    
    # Đưa về CHW: [1, 3, 112, 112]
    img = np.transpose(img, (2, 0, 1))
    img = np.expand_dims(img, axis=0)
    
    return img

def cosine_similarity(emb1, emb2):
    """
    Tính độ tương đồng Cosine giữa 2 vector đặc trưng (embeddings).
    Giá trị trả về nằm trong khoảng [-1, 1], càng gần 1 nghĩa là càng giống nhau.
    """
    dot_product = np.dot(emb1, emb2)
    norm1 = np.linalg.norm(emb1)
    norm2 = np.linalg.norm(emb2)
    if norm1 == 0 or norm2 == 0:
        return 0.0
    return dot_product / (norm1 * norm2)

def extract_embedding(session, img_array):
    """
    Chạy suy luận ONNX model để lấy vector 192 chiều từ tensor embeddings.
    """
    input_name = session.get_inputs()[0].name
    output_name = session.get_outputs()[0].name
    
    # Chạy inference
    outputs = session.run([output_name], {input_name: img_array})
    embedding = outputs[0] # Shape: [1, 192]
    
    # Làm phẳng mảng thành 1D vector và chuẩn hóa L2 (L2 normalize) để tính cosine chính xác hơn
    embedding = embedding.flatten()
    embedding = embedding / np.linalg.norm(embedding)
    return embedding

def compare_two_faces(model_path, image_path_1, image_path_2, threshold=0.6):
    print(f"Đang tải model ONNX từ: {model_path}...")
    session = ort.InferenceSession(model_path, providers=['CPUExecutionProvider'])
    
    print(f"Đang xử lý ảnh 1: {image_path_1}")
    img1 = preprocess_image(image_path_1)
    
    print(f"Đang xử lý ảnh 2: {image_path_2}")
    img2 = preprocess_image(image_path_2)
    
    print("Đang trích xuất đặc trưng (Embeddings)...")
    emb1 = extract_embedding(session, img1)
    emb2 = extract_embedding(session, img2)
    
    # Tính độ tương đồng
    similarity = cosine_similarity(emb1, emb2)
    
    print("\n================ KẾT QUẢ SO SÁNH ================")
    print(f"- Độ tương đồng (Cosine Similarity): {similarity:.4f}")
    
    if similarity >= threshold:
        print(f"-> KẾT LUẬN: HAI ẢNH LÀ **CÙNG MỘT NGƯỜI** (Ngưỡng >= {threshold})")
    else:
        print(f"-> KẾT LUẬN: HAI ẢNH LÀ **HAI NGƯỜI KHÁC NHAU** (Ngưỡng < {threshold})")
    print("==================================================")

if __name__ == "__main__":
    # Thay đổi đường dẫn model và 2 ảnh cần so sánh của bạn vào đây
    MODEL_ONNX = "../models/mobilefacenet.onnx"
    IMG_1 = "../data/face_test/b1.jpg"
    IMG_2 = "../data/face_test/b2.jpg"
    
    compare_two_faces(MODEL_ONNX, IMG_1, IMG_2, threshold=0.55)