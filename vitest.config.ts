import { defineConfig } from 'vitest/config'
import {fileURLToPath} from 'node:url'
export default defineConfig({resolve:{alias:{'~':fileURLToPath(new URL('./apps/ui/src',import.meta.url)), '~brand':fileURLToPath(new URL('./data/branding',import.meta.url))}}, test: {globals:true, include: ['tests/desktop/**/*.test.ts','tests/interop/**/*.test.ts','tests/ui/**/*.test.{ts,tsx}','apps/ui/src/**/*.test.{ts,tsx}'], environment: 'node', environmentMatchGlobs:[['apps/ui/**','jsdom'],['tests/ui/**','jsdom']],testTimeout: 15000, pool: 'forks', maxWorkers: 3 } })
