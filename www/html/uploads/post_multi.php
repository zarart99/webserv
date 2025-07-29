<?php

function isTextFile($filePath) {
	$sample = file_get_contents($filePath, false, null, 0, 512);
	return preg_match('//u', $sample) && !preg_match('/[^\x09\x0A\x0D\x20-\x7E]/', $sample);//проверяем на нечитаемые символы
}

if ($_SERVER["REQUEST_METHOD"] == "POST") {
	if ($_FILES['myfile']['error'] === UPLOAD_ERR_OK) {
		$tmp = $_FILES['myfile']['tmp_name'];
		$name = basename($_FILES['myfile']['name']);
		$upload_dir = getenv('UPLOAD_PATH') ?: __DIR__ . "/upl";
		$dest =  $upload_dir . "/" . $name;

		if (!is_dir($upload_dir)) {
			mkdir($upload_dir, 0777, true);
		}

		if (move_uploaded_file($tmp, "$dest"))
		{
			echo "<p>Fichier reçu: $name</p>";
			$mime = mime_content_type($dest);

			if (str_starts_with($mime, "image/")) {
				$data = base64_encode(file_get_contents($dest));
				echo "<img src='data:$mime;base64,$data' style='max-width:400px;' />";
			} elseif (isTextFile($dest)) {
				$contenu = htmlspecialchars(file_get_contents("$dest")) ;
				echo "<pre>$contenu</pre>";
			} else {
				echo "<p>Error type of file : $mime</p>";
			}

		} else {
			echo "<p>Error move_uploaded_file</p>";
		}
	} else {
		echo "<p>Error upload : code " . $_FILES['myfile']['error'] . "</p>";
	}
}
?>




