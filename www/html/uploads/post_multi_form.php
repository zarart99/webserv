<!DOCTYPE html>
<html>
<head><title>Formulaire POST Multi</title></head>
<body>
  <form method="POST" action="http://localhost:8001/uploads/post_multi.php" enctype="multipart/form-data">
	<input type="file" name="myfile">
	<button type="submit">Envoyer</button>
  </form>
</body>
</html>