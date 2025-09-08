from pymongo import MongoClient
import sys

# Obtém o GUID da câmera a partir dos argumentos da linha de comando
if len(sys.argv) < 2:
    print("Uso: python 02_mongo_db_read.py <GUID_DA_CAMERA>")
    sys.exit(1)
camera_guid = sys.argv[1]

# Conecta ao MongoDB local
client = MongoClient("mongodb://localhost:27017/")
db = client["testeDB"]
collection = db["reconhecimento"]

print(f"\n📥 Últimos 10 documentos para GUID: {camera_guid}")
for doc in collection.find({"guid": camera_guid}).sort("timestamp", -1).limit(10):
    print(doc)
    print("---------------------------------")
