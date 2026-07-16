function monitorForm() {
	const username = document.getElementById("name");
	const message = document.getElementById("message");
	const form = document.getElementById("contact-form");

	form.addEventListener("submit", async (e) => {
		e.preventDefault();
		await sendMessage(username.value, message.value);
	});
}

async function sendMessage(username, message) {
	try {
		const response = await fetch("/messages/", {
			method: "POST",
			body: JSON.stringify({ name: username, message: message }),
			headers: {
				"Content-Type": "application/json",
			},
		});
		updateView(response);
	} catch {
		updateView();
	}
}

function updateView(response) {
	const contactSection = document.getElementById("contact");
	const resultP = document.createElement("p");
	resultP.id = "result";
	const existingResult = document.getElementById(resultP.id);
	if (existingResult) existingResult.remove();
	if (!response)
		resultP.textContent = "Unknown error while sending message";
	else if (response.ok) resultP.textContent = "Message sent successfully!";
	else {
		resultP.textContent = `Error while sending message: ${response.statusText}`;
		resultP.classList.add("error");
	}
	contactSection.appendChild(resultP);
}

monitorForm();
