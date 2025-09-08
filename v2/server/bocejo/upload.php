<?php

date_default_timezone_set('America/Sao_Paulo'); // Garante horário local correto

$logFile = __DIR__ . '/upload_errors.txt';

try {
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_FILES['image'])) {
        $uploadDir = __DIR__ . '/';
        $fileTmpPath = $_FILES['image']['tmp_name'];
        $originalName = $_FILES['image']['name'];
        $extension = strtolower(pathinfo($originalName, PATHINFO_EXTENSION));

        // Captura parâmetro da URL
        $urlParam = isset($_GET['text']) ? $_GET['text'] : 'sem-parametro';

        // Gera nome com data, "text" e parâmetro
        $timestamp = date('Y-m-d-H-i-s');
        $safeParam = preg_replace('/[^a-zA-Z0-9-_]/', '_', $urlParam); // sanitiza
        $newFileName = $timestamp . '-text-' . $safeParam . '.' . ($extension === 'jpeg' ? 'jpeg' : $extension);
        $destination = $uploadDir . $newFileName;

        // Move o arquivo
        if (move_uploaded_file($fileTmpPath, $destination)) {
            echo "✅ Upload bem-sucedido: $newFileName";
        } else {
            throw new Exception("Erro ao salvar o arquivo.");
        }
    } else {
        throw new Exception("Nenhuma imagem recebida.");
    }
} catch (Exception $e) {
    // Registra erro no log
    file_put_contents($logFile, "[" . date('Y-m-d H:i:s') . "] " . $e->getMessage() . "\n", FILE_APPEND);
    echo "❌ Erro: " . $e->getMessage();
}
?>

