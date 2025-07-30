<?php
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
	$msg = htmlspecialchars($_POST['message'] ?? '');
	echo "<p>Message received :</p>";
	echo "<pre>$msg</pre>";
} else {
	echo "<p>Error : unauthorized method</p>";
}
?>