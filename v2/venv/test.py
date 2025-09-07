# ok inclinação da cabeça
# ok bocejo
# ok olho fechado
# ok mão na frente do rosto
# ok mao proxima ao rosto
# ok cabeça baixa
# ok olhos muito abertos
# mais de uma pessoa

import cv2
import mediapipe as mp
import numpy as np
import requests
import time
import os
from pymongo import MongoClient



mp_face_mesh = mp.solutions.face_mesh
face_mesh = mp_face_mesh.FaceMesh(static_image_mode=True)
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(static_image_mode=True, max_num_hands=2)
mp_face_detection = mp.solutions.face_detection
face_detection = mp_face_detection.FaceDetection(model_selection=1, min_detection_confidence=0.7)

# 3D model points for head pose estimation
face_model_3d = np.array([
    (0.0, 0.0, 0.0),             # Nose tip
    (0.0, -330.0, -65.0),        # Chin
    (-225.0, 170.0, -135.0),     # Left eye left corner
    (225.0, 170.0, -135.0),      # Right eye right corner
    (-150.0, -150.0, -125.0),    # Left mouth corner
    (150.0, -150.0, -125.0)      # Right mouth corner
], dtype=np.float64)


def enviar_objeto_para_mongodb(obj):
    client = MongoClient("mongodb://localhost:27017/")
    db = client["testeDB"]
    collection = db["leituras"]
    collection.insert_one(obj)


def get_head_pose_euler_angles(img_w, img_h, face_landmarks):
    # 2D image points from the face landmarks
    # We use specific landmarks for nose tip, chin, eye corners, and mouth corners
    # These indices correspond to the MediaPipe Face Mesh model
    image_points = np.array([
        (face_landmarks.landmark[1].x * img_w, face_landmarks.landmark[1].y * img_h),    # Nose tip (index 1)
        (face_landmarks.landmark[199].x * img_w, face_landmarks.landmark[199].y * img_h), # Chin (index 199)
        (face_landmarks.landmark[33].x * img_w, face_landmarks.landmark[33].y * img_h),   # Left eye left corner (index 33)
        (face_landmarks.landmark[263].x * img_w, face_landmarks.landmark[263].y * img_h),  # Right eye right corner (index 263)
        (face_landmarks.landmark[61].x * img_w, face_landmarks.landmark[61].y * img_h),   # Left mouth corner (index 61)
        (face_landmarks.landmark[291].x * img_w, face_landmarks.landmark[291].y * img_h)  # Right mouth corner (index 291)
    ], dtype=np.float64)

    # Camera internals - approximate values (can be calibrated for better accuracy)
    focal_length = 1 * img_w
    cam_center = (img_w / 2, img_h / 2)
    camera_matrix = np.array(
        [[focal_length, 0, cam_center[0]],
         [0, focal_length, cam_center[1]],
         [0, 0, 1]], dtype=np.float64
    )
    dist_coeffs = np.zeros((4, 1)) # No lens distortion

    (success, rotation_vector, translation_vector) = cv2.solvePnP(
        face_model_3d, image_points, camera_matrix, dist_coeffs, flags=cv2.SOLVEPNP_ITERATIVE
    )

    if not success:
        return None, None, None

    # Get rotation matrix
    rmat, _ = cv2.Rodrigues(rotation_vector)

    # Get Euler angles (pitch, yaw, roll)
    # Pitch: up/down tilt
    # Yaw: left/right turn
    # Roll: tilt left/right
    angles, _, _, _, _, _, _ = cv2.decomposeProjectionMatrix(
        np.concatenate((rmat, translation_vector), axis=1)
    )

    pitch = angles[0][0] # Y-axis rotation (up/down)
    yaw = angles[1][0]   # X-axis rotation (left/right)
    roll = angles[2][0]  # Z-axis rotation (tilt left/right)

    return pitch, yaw, roll

def eye_aspect_ratio(landmarks, eye_indices):
    # Extract the coordinates of the eye landmarks
    p1 = np.array([landmarks[eye_indices[0]].x, landmarks[eye_indices[0]].y])
    p2 = np.array([landmarks[eye_indices[1]].x, landmarks[eye_indices[1]].y])
    p3 = np.array([landmarks[eye_indices[2]].x, landmarks[eye_indices[2]].y])
    p4 = np.array([landmarks[eye_indices[3]].x, landmarks[eye_indices[3]].y])
    p5 = np.array([landmarks[eye_indices[4]].x, landmarks[eye_indices[4]].y])
    p6 = np.array([landmarks[eye_indices[5]].x, landmarks[eye_indices[5]].y])

    # Compute the euclidean distances between the two sets of vertical eye landmarks (x, y)-coordinates
    A = np.linalg.norm(p2 - p6)
    B = np.linalg.norm(p3 - p5)

    # Compute the euclidean distance between the horizontal eye landmark (x, y)-coordinates
    C = np.linalg.norm(p1 - p4)

    # Compute the eye aspect ratio
    ear = (A + B) / (2.0 * C)

    return ear

def olho_fechado(caminho_imagem):
    imagem = cv2.imread(caminho_imagem)
    if imagem is None:
        #print("Imagem não encontrada.")
        return None # Return None if image not found to indicate failure

    imagem_rgb = cv2.cvtColor(imagem, cv2.COLOR_BGR2RGB)
    resultado = face_mesh.process(imagem_rgb)

    if not resultado.multi_face_landmarks:
        #print("Nenhum rosto detectado.")
        return None # Return None if no faces detected

    # We'll return the EAR of the *first* detected face for simplicity in this general function.
    # More complex logic could average across all faces or return a list of EARs.
    rosto_landmarks = resultado.multi_face_landmarks[0]
    
    # Left eye landmarks (MediaPipe Face Mesh indices)
    LEFT_EYE = [33, 160, 158, 133, 153, 144]
    # Right eye landmarks (MediaPipe Face Mesh indices)
    RIGHT_EYE = [362, 385, 387, 263, 373, 380]

    left_ear = eye_aspect_ratio(rosto_landmarks.landmark, LEFT_EYE)
    right_ear = eye_aspect_ratio(rosto_landmarks.landmark, RIGHT_EYE)

    avg_ear = (left_ear + right_ear) / 2.0
    return avg_ear

def enviar_imagem_para_servidor(imagem_path):
    url = "http://incar.gsalute.com.br/bocejo/upload.php"
    try:
        with open(imagem_path, 'rb') as f:
            files = {'image': (imagem_path, f, 'image/png')}
            resposta = requests.post(url, files=files, timeout=10)
        if resposta.status_code == 200:
            #print("📤 Imagem enviada com sucesso!")
            x = 0
        else:
            #print(f"⚠️ Erro ao enviar imagem: {resposta.status_code}")
            x = 0
    except Exception as e:
        print("❌ Falha ao enviar imagem:", e)


def detectar_bocejo(imagem_path):
    imagem = cv2.imread(imagem_path)
    if imagem is None:
        #print("❌ Imagem não encontrada:", imagem_path)
        return False

    imagem_rgb = cv2.cvtColor(imagem, cv2.COLOR_BGR2RGB)
    resultado = face_mesh.process(imagem_rgb)

    if not resultado.multi_face_landmarks:
        #print("😐 Nenhum rosto detectado.")
        return False
    else:
        #print(f"qtdo{len(resultado.multi_face_landmarks)} resto(s)")
        x = 0


    #print(len(resultado.multi_face_landmarks))
    for rosto in resultado.multi_face_landmarks:
        pontos = rosto.landmark

        labio_superior = pontos[13]
        labio_inferior = pontos[14]

        altura_boca = np.linalg.norm(
            np.array([labio_superior.x, labio_superior.y]) -
            np.array([labio_inferior.x, labio_inferior.y])
        )

        #print(f"Abertura da boca: {altura_boca:.4f}")
        if altura_boca > 0.03:
            #print("🟢 Bocejo detectado!")
            return True
        else:
            #print("🔵 Boca fechada ou normal.")
            x = 0

    return False # No yawn detected after checking all faces


def baixar_imagem(url, destino):
    try:
        resposta = requests.get(url, timeout=10)
        if resposta.status_code == 200:
            with open(destino, 'wb') as f:
                f.write(resposta.content)
            return True
        else:
            #print(f"⚠️ Erro HTTP: {resposta.status_code}")
            return False
    except Exception as e:
        #print("❌ Falha ao baixar imagem:", e)
        return False

def detect_head_tilt(image_path, up_threshold=10, down_threshold=-10):
    image = cv2.imread(image_path)
    if image is None:
        #print("❌ Imagem não encontrada:", image_path)
        return False

    image_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    results = face_mesh.process(image_rgb)

    if not results.multi_face_landmarks:
        #print("Nenhum rosto detectado para análise de inclinação da cabeça.")
        return False

    for face_landmarks in results.multi_face_landmarks:
        img_h, img_w, _ = image.shape
        pitch, yaw, roll = get_head_pose_euler_angles(img_w, img_h, face_landmarks)

        if pitch is not None:
            #print(f"Inclinação da cabeça (Pitch): {pitch:.2f} graus")
            if pitch > up_threshold:
                #print("⚠️ Cabeça inclinada para cima detectada!\n")
                return True
            elif pitch < down_threshold:
                #print("⚠️ Cabeça inclinada para baixo detectada!\n")
                return True
    
    #print("✅ Nenhuma inclinação de cabeça significativa detectada.")
    return False

def detect_tiredness_or_yawn(image_path):
    # tired = olho_fechado(image_path) # Removed closed eye check
    yawn = detectar_bocejo(image_path)

    # if tired:
    #     print("⚠️ Fadiga detectada (olhos fechados)!\n")
    #     return True
    if yawn:
        #print("⚠️ Bocejo detectado!\n")
        return True
    #print("✅ Nenhum sinal de fadiga ou bocejo detectado.")
    return False

def validate_closed_eyes(image_path, ear_threshold=0.18):
    avg_ear = olho_fechado(image_path)
    if avg_ear is not None and avg_ear < ear_threshold:
        #print(f"⚠️ Olhos fechados detectados! EAR médio: {avg_ear:.4f}")
        return True
    #print("✅ Olhos abertos ou não detectados.")
    return False

def validate_wide_open_eyes(image_path, wide_open_threshold=0.40):
    avg_ear = olho_fechado(image_path)
    if avg_ear is not None and avg_ear > wide_open_threshold:
        #print(f"⚠️ Olhos bem abertos detectados! EAR médio: {avg_ear:.4f}")
        return True
    #print("✅ Olhos em estado normal ou não detectados como muito abertos.")
    return False

def detect_hand_in_front_of_face(image_path):
    image = cv2.imread(image_path)
    if image is None:
        #print("❌ Imagem não encontrada:", image_path)
        return False

    image_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    face_results = face_mesh.process(image_rgb)
    hand_results = hands.process(image_rgb)

    if not face_results.multi_face_landmarks:
        return False

    if hand_results.multi_hand_landmarks:
        for face_landmarks in face_results.multi_face_landmarks:
            # Get face bounding box
            face_x_min, face_y_min = image.shape[1], image.shape[0]
            face_x_max, face_y_max = 0, 0
            for lm in face_landmarks.landmark:
                x, y = int(lm.x * image.shape[1]), int(lm.y * image.shape[0])
                face_x_min = min(face_x_min, x)
                face_y_min = min(face_y_min, y)
                face_x_max = max(face_x_max, x)
                face_y_max = max(face_y_max, y)

            for hand_landmarks in hand_results.multi_hand_landmarks:
                # Get hand bounding box
                hand_x_min, hand_y_min = image.shape[1], image.shape[0]
                hand_x_max, hand_y_max = 0, 0
                for lm in hand_landmarks.landmark:
                    x, y = int(lm.x * image.shape[1]), int(lm.y * image.shape[0])
                    hand_x_min = min(hand_x_min, x)
                    hand_y_min = min(hand_y_min, y)
                    hand_x_max = max(hand_x_max, x)
                    hand_y_max = max(hand_y_max, y)

                # Check for overlap
                if (hand_x_max > face_x_min and hand_x_min < face_x_max and
                        hand_y_max > face_y_min and hand_y_min < face_y_max):
                    #print("⚠️ Mão detectada na frente do rosto!\n")
                    return True
    #print("✅ Nenhuma mão detectada na frente do rosto.")
    return False

def detect_multiple_faces(image_path):
    imagem = cv2.imread(image_path)
    if imagem is None:
        #print("❌ Imagem não encontrada:", image_path)
        return False

    imagem_rgb = cv2.cvtColor(imagem, cv2.COLOR_BGR2RGB)
    # Use face_detection instead of face_mesh for general face detection
    resultado = face_detection.process(imagem_rgb)

    if resultado.detections and len(resultado.detections) > 1:
        #print(f"⚠️ {len(resultado.detections)} rostos detectados!\n")
        return True
    else:
        #print("✅ Apenas um rosto ou nenhum rosto detectado.")
        return False

def main():
    while True:
        nome_arquivo = "imagem_recebida.png"
        url_download = "http://incar.gsalute.com.br/download.php"

        #print("\n🔄 Baixando imagem...")
        if baixar_imagem(url_download, nome_arquivo):
            process_image(nome_arquivo)
            os.remove(nome_arquivo)  # Remove após análise
        else:
            #print("⏳ Aguardando próxima tentativa...")
            x = 0
        #time.sleep(1)


def process_image(image_path):
    should_upload = False

    maisDeUmaPessoa = False
    maoProximaAoRosto = False
    olhosMuitoAbertos = False
    cabecaBaixa = False
    olhosFechados = False
    bocejo = False

    if detect_multiple_faces(image_path):
        maisDeUmaPessoa = True
        should_upload = True

    if detect_hand_in_front_of_face(image_path):
        maoProximaAoRosto = True
        should_upload = True

    if detect_tiredness_or_yawn(image_path):
        bocejo = True
        should_upload = True

    if validate_closed_eyes(image_path):
        olhosFechados = True
        should_upload = True

    if detect_head_tilt(image_path):
        cabecaBaixa = True
        should_upload = True
    
    if validate_wide_open_eyes(image_path):
        olhosMuitoAbertos = True
        should_upload = True

    if should_upload:
        enviar_imagem_para_servidor(image_path)
        
    
    doc = {
        "timestamp": datetime.now(),
        "maisDeUmaPessoa": maisDeUmaPessoa,
        "maoProximaAoRosto": maoProximaAoRosto,
        "olhosMuitoAbertos": olhosMuitoAbertos,
        "cabecaBaixa": cabecaBaixa,
        "olhosFechados": olhosFechados,
        "bocejo": bocejo
    }
    enviar_objeto_para_mongodb(doc)


    print(f"maisDeUmaPessoa\t\t: {maisDeUmaPessoa}")
    print(f"maoProximaAoRosto\t: {maoProximaAoRosto}")
    print(f"olhosMuitoAbertos\t: {olhosMuitoAbertos}")
    print(f"cabecaBaixa\t\t: {cabecaBaixa}")
    print(f"olhosFechados\t\t: {olhosFechados}")
    print(f"bocejo\t\t\t: {bocejo}")
    print("" * 2)
    print("--------------------------------")
    print("" * 2)


if __name__ == "__main__":
    main()

