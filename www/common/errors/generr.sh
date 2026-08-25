#!/bin/bash


if [ -z "$1" ];then
	echo "usage : ./generr.sh CODE Error Message > CODE.html"
	exit
fi

CODE=$1
shift

cat << EOF
<!doctype html>
<html lang="en">

<head>
	<meta charset="UTF-8" />
	<meta name="viewport" content="width=device-width, initial-scale=1.0" />
	<link rel="stylesheet" href="/css/styles.css" />
	<title>Webserv Error Page</title>
</head>

<body>
	<header>
		<h1>WEBSERV</h1>
		<p>Demonstration Website</p>
		<p>for our HTTP server</p>
	</header>
	<main>
		<section id="error-section">
EOF
	echo "<h1>$CODE</h1>"
	echo "<h2>$@</h2>"
cat << EOF
		</section>
	</main>
	<footer>
		<div>© Kevin Donlon, Noa Houssier, Maxime Marti</div>
		<a href="https://github.com/NestorNebula/webserv" target="_blank" rel="noopener noreferrer">
			<img id="gh-icon" src="/assets/icons/github.svg" alt="GitHub" />
		</a>
		<div>2026</div>
	</footer>
</body>

</html>
EOF