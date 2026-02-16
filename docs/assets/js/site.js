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

  if (document.body) {
    document.body.setAttribute("data-md-color-scheme", "slate");
    document.body.setAttribute("data-md-color-primary", "teal");
    document.body.setAttribute("data-md-color-accent", "lime");
  }
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
