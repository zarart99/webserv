<?php
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
	$msg = $_POST['message'];
	echo "<h1>Message reçu: $msg</h1>";
}
?>