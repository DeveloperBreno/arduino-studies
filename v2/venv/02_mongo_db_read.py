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

# Initialize counters for the attributes
attribute_counts = {
    "maisDeUmaPessoa": 0,
    "maoProximaAoRosto": 0,
    "olhosMuitoAbertos": 0,
    "cabecaBaixa": 0,
    "olhosFechados": 0,
    "bocejo": 0
}

print(f"\n📥 Últimos 10 documentos para GUID: {camera_guid}")

# Fetch and process the last 10 documents
for doc in collection.find({"guid": camera_guid}).sort("timestamp", -1).limit(10):
    print(doc)
    print("---------------------------------")

    # Increment counters if attribute is True
    for attr in attribute_counts.keys():
        if doc.get(attr, False):
            attribute_counts[attr] += 1

print("\n📊 Frequência dos atributos nos últimos 10 registros:")
for attr, count in attribute_counts.items():
    print(f"{attr}: {count} ocorrência(s)")
