CI Note

The repo-wide linter appears to fail due to a malformed script at app-gateway/.init/.linter.sh. 
This App2AppProvider package provides its own lint/build commands under package.json (eslint + tsc).
Use:
- npm ci || npm i
- npm run lint
- npm run build

These commands run only within this package and do not invoke the broken repo-wide shell script.
