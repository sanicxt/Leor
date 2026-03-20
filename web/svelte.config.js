import adapter from '@sveltejs/adapter-static';
import { vitePreprocess } from '@sveltejs/vite-plugin-svelte';

const githubRepository = process.env.GITHUB_REPOSITORY;
const [githubOwner, githubRepo] = (githubRepository ?? '').split('/');

const inferredBase =
	githubOwner && githubRepo && githubRepo !== `${githubOwner}.github.io` ? `/${githubRepo}` : '';

const base = process.env.BASE_PATH ?? inferredBase;

/** @type {import('@sveltejs/kit').Config} */
const config = {
	// Consult https://svelte.dev/docs/kit/integrations
	// for more information about preprocessors
	preprocess: vitePreprocess(),

	kit: {
		paths: {
			base
		},
		adapter: adapter({
			pages: 'build',
			assets: 'build',
			fallback: '404.html',
			precompress: false,
			strict: true
		})
	}
};

export default config;
