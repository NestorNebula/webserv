function displayCookie() {
  const cookie = getCookie("wstimecookie");
  const firstVisit = document.getElementById("cookie-first-visit");
  const lastVisit = document.getElementById("cookie-last-visit");
  const cookieError = document.getElementById("cookie-error");

  if (!cookieError || !firstVisit || !lastVisit)
    return;
  if (!cookie) {
    cookieError.textContent = "Webserv cookie not found";
    return;
  }
  const visitsTimes = cookie.split("|");
  if (visitsTimes.length != 2)
    return setError(cookieError, "Invalid cookie format");
  const firstTime = new Date(0);
  const lastTime = new Date(0);
  firstTime.setUTCSeconds(visitsTimes[0]);
  lastTime.setUTCSeconds(visitsTimes[1]);
  if (isNaN(firstTime.getTime()) || isNaN(lastTime.getTime()))
    return setError(cookieError, "Invalid date format");
  firstVisit.textContent = "First Visit: " + firstTime.toLocaleString();
  lastVisit.textContent = "Last Visit: " + lastTime.toLocaleString();
}

function getCookie(name = "cookie") {
  const cookies = document.cookie.split(";");
  for (let i = 0; i < cookies.length; i++) {
    const cookie = cookies[i].trim();
    if (cookie.startsWith(name + "="))
      return cookie.substring(cookie.indexOf("=") + 1);
  }
}

function setError(errorElement, error = "Error in cookie processing") {
  if (!errorElement)
    return;
  errorElement.textContent = error;
}

displayCookie();
