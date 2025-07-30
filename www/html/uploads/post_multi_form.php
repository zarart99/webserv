<!DOCTYPE html>
<html>
<head><title>Formulaire POST multi TEXT/IMG</title></head>
<body>
  <h2>Upload a file</h2>
  <form method="POST" action="http://localhost:8001/uploads/post_multi.php" enctype="multipart/form-data">
	<input type="file" name="myfile"><br><br>
	<button type="submit">Send file</button>
  </form>
</body>
</html>