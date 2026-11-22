import { defineConfig, type UserConfig } from 'electron-vite'
import { resolve } from 'node:path'
import uiConfig from '../ui/vite.config'
const config: UserConfig = {
  main: {build:{outDir:'dist/main',rollupOptions:{input:resolve(__dirname,'src/main/index.ts')}}},
  preload: {build:{outDir:'dist/preload',rollupOptions:{input:resolve(__dirname,'src/preload/index.ts'),output:{format:'cjs',entryFileNames:'index.cjs'}}}},
  renderer: {...uiConfig,root:resolve(__dirname,'../ui'),build:{...uiConfig.build,minify:'esbuild',cssMinify:true,outDir:resolve(__dirname,'../ui/dist'),rollupOptions:{...uiConfig.build?.rollupOptions,input:resolve(__dirname,'../ui/index.html')}}}
}
// Daily shell edits do not rebuild the React renderer. The ordinary CLI keeps all targets.
export default defineConfig(process.env.MOUSEBOX_ELECTRON_BUILD_SCOPE === 'shell'
  ? { main: config.main, preload: config.preload }
  : config)
