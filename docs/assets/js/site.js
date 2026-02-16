(() => {
  const darkPalette = {
    color: {
      media: "",
      scheme: "slate",
      primary: "teal",
      accent: "lime",
    },
  };

  if (typeof window.__md_set === "function") {
    window.__md_set("__palette", darkPalette);
  }

  try {
    localStorage.setItem("__palette", JSON.stringify(darkPalette));
  } catch (_err) {
    // Ignore storage write failures (private mode, blocked storage, etc.).
  }

  const root = document.documentElement;
  root.setAttribute("data-md-color-scheme", "slate");
  root.setAttribute("data-md-color-primary", "teal");
  root.setAttribute("data-md-color-accent", "lime");
})();

document.addEventListener("DOMContentLoaded", () => {
  const article = document.querySelector(".md-content__inner");
  if (article) {
    article.classList.add("page-fade-in");
  }

  const staggerRoots = document.querySelectorAll(".hero-grid, .stagger-root");
  staggerRoots.forEach((root) => {
    Array.from(root.children).forEach((child) => {
      child.classList.add("stagger-item");
    });
  });
});
