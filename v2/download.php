<?php

// usar venv: source venv_bocejo/bin/activate


$dir = __DIR__;
$logFile = $dir . '/download_errors.txt';

try {
    // Lista todos os arquivos .png
    $files = glob($dir . '/*.png');

    if (!$files || count($files) === 0) {
        throw new Exception("Nenhum arquivo PNG encontrado.");
    }

    // Ordena por data de modificação (mais antigo primeiro)
    usort($files, function($a, $b) {
        return filemtime($a) - filemtime($b);
    });

    $oldestFile = $files[0];
    $filename = basename($oldestFile);

    // Define cabeçalhos para download
    header('Content-Description: File Transfer');
    header('Content-Type: image/png');
    header('Content-Disposition: attachment; filename="' . $filename . '"');
    header('Content-Length: ' . filesize($oldestFile));
    header('Cache-Control: no-cache, must-revalidate');
    header('Expires: 0');

    // Envia o arquivo
    readfile($oldestFile);

    // Exclui após envio
    unlink($oldestFile);
} catch (Exception $e) {
    // Log de erro
    file_put_contents($logFile, "[" . date('Y-m-d H:i:s') . "] " . $e->getMessage() . "\n", FILE_APPEND);
    http_response_code(404);
    echo "❌ Erro: " . $e->getMessage();
}
?>

