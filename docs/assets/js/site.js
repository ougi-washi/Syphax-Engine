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
