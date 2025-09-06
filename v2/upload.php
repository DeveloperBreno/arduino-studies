<?php
// Verifica se um arquivo foi enviado via POST
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_FILES['image'])) {
    $uploadDir = __DIR__ . '/'; // Diretório atual
    $fileTmpPath = $_FILES['image']['tmp_name'];
    $fileName = basename($_FILES['image']['name']);
    $destination = $uploadDir . $fileName;

    // Move o arquivo para o diretório atual
    if (move_uploaded_file($fileTmpPath, $destination)) {
        echo "Upload bem-sucedido: $fileName";
    } else {
        echo "Erro ao salvar o arquivo.";
    }
} else {
    echo "Nenhuma imagem recebida.";
}
?>

