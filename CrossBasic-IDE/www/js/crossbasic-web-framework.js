function fetchFunctionCall(functionName, params = {}, onSuccess, onError) {
  let url = `http://localhost:8080/call?function=${encodeURIComponent(functionName)}`;

  for (const [key, value] of Object.entries(params)) {
    if (value !== undefined && value !== null) {
      url += `&${encodeURIComponent(key)}=${encodeURIComponent(value)}`;
    }
  }

  fetch(url)
    .then(response => response.json())
    .then(data => {
      if (data.redirect) {
        window.location.href = data.redirect;
      } else if (data.result !== undefined) {
        onSuccess?.(data.result);
      } else if (data.error !== undefined) {
        onError?.(data.error);
      }
    })
    .catch(err => onError?.(err.message || "An error occurred"));
}