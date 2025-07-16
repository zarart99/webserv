<?php

if ($_SERVER["REQUEST_METHOD"] == "POST") {
	if ($_FILES['myfile']['error'] === UPLOAD_ERR_OK) {
		$tmp = $_FILES['myfile']['tmp_name'];
		$name = $_FILES['myfile']['name'];
		$dest =  __DIR__ . "/upl/" . basename($name);

		if (move_uploaded_file($tmp, "$dest"))
		{
			$contenu = file_get_contents("$dest");
			echo "<p>Fichier reçu: $name</p>";
			echo "<pre>" . htmlspecialchars($contenu) . "</pre>";
		} else {
			echo "<p>Error move_uploaded_file</p>";
		}
	} else {
		echo "<p>Error lors de l'upload : code </p>" . $_FILES['myfile']['error'] . "</p>";
	}
}
?>