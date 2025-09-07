from pymongo import MongoClient
from datetime import datetime
import time

# Conecta ao MongoDB local
client = MongoClient("mongodb://localhost:27017/")
db = client["testeDB"]
collection = db["leituras"]

# Inserção de múltiplos documentos
for i in range(10):
    doc = {
        "timestamp": datetime.now(),
        "valor": i,
        "status": "ok" if i % 2 == 0 else "erro"
    }
    collection.insert_one(doc)
    print(f"✅ Inserido: {doc}")
    time.sleep(0.5)  # Simula intervalo entre leituras

# Leitura dos últimos documentos
print("\n📥 Documentos com status 'ok':")
for doc in collection.find({"status": "ok"}).sort("timestamp", -1).limit(5):
    print(doc)
