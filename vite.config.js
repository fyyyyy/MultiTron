import { defineConfig } from "vite";

export default defineConfig({
  root: "src/UI",
  server: {
    host: "127.0.0.1",
    port: 4173,
    strictPort: true,
  },
  plugins: [
    {
      name: "use-typescript-components-in-dev",
      transformIndexHtml: {
        order: "pre",
        handler(html) {
          return html.replace(
            '<script defer src="components.js"></script>',
            '<script type="module" src="/components.ts"></script>',
          );
        },
      },
    },
  ],
});
