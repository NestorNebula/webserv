function monitorForm() {
  const uri = document.getElementById("uri");
	const form = document.getElementById("delete-form");

	form.addEventListener("submit", async (e) => {
		e.preventDefault();
		await sendRequest(uri.value);
		uri.value = "";
	});
}

async function sendRequest(uri) {
	try {
		const response = await fetch(uri, {
			method: "DELETE",
		});
		updateView(response);
	} catch {
		updateView();
	}
}

function updateView(response) {
	const contactSection = document.getElementById("delete");
	const resultP = document.createElement("p");
	resultP.id = "result";
	const existingResult = document.getElementById(resultP.id);
	if (existingResult) existingResult.remove();
	if (!response)
		resultP.textContent = "Unknown error while sending deletion request";
	else if (response.ok) resultP.textContent = "File deleted successfully!";
	else {
		resultP.textContent = `File couldn't be deleted: ${response.statusText}`;
		resultP.classList.add("error");
	}
	contactSection.appendChild(resultP);
}

monitorForm();
