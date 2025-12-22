# Svelte library

Everything you need to build a Svelte library, powered by [`sv`](https://npmjs.com/package/sv).

Read more about creating a library [in the docs](https://svelte.dev/docs/kit/packaging).

## Creating a project

If you're seeing this, you've probably already done this step. Congrats!

```sh
# create a new project in the current directory
npx sv create

# create a new project in my-app
npx sv create my-app
```

## Developing

Once you've created a project and installed dependencies with Bun, start a development server:

```sh
bun install
bun run dev

# or start the server and open the app in a new browser tab
npm run dev -- --open
```

Everything inside `src/lib` is part of your library, everything inside `src/routes` can be used as a showcase or preview app.

## Building

To build your library:

```sh
npm pack
```

To create a production version of your showcase app:

```sh
npm run build
```

## GitHub Pages

If you publish this app to GitHub Pages from a repository that is **not** named `your-username.github.io`, the site is served from a subpath:

`https://your-username.github.io/your-repo-name/`

This project is configured to:

- Automatically set `config.kit.paths.base` to `/${repo}` when `GITHUB_REPOSITORY=owner/repo` is present.
- Generate a fallback `404.html` (via `adapter-static`) so client-side routing works on refresh.

Build for Pages from the `leora/` folder:

```sh
bun install
bun run build:pages
```

### GitHub Actions deploy

This repo includes a workflow that builds and deploys to GitHub Pages on pushes to `main`:

- Workflow: `.github/workflows/deploy.yml`
- Output folder: `leora/build/`

Optional override (if you want to force a specific base path):

```sh
BASE_PATH=/your-repo-name bun run build:pages
```

You can preview the production build with `npm run preview`.

> To deploy your app, you may need to install an [adapter](https://svelte.dev/docs/kit/adapters) for your target environment.

## Publishing

Go into the `package.json` and give your package the desired name through the `"name"` option. Also consider adding a `"license"` field and point it to a `LICENSE` file which you can create from a template (one popular option is the [MIT license](https://opensource.org/license/mit/)).

To publish your library to [npm](https://www.npmjs.com):

```sh
npm publish
```
