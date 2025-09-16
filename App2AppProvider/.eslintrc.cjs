module.exports = {
  root: true,
  env: {
    es2022: true,
    node: true,
  },
  // Use eslint-config-prettier last to disable any formatting rules that may conflict with Prettier
  extends: ['eslint:recommended', 'plugin:@typescript-eslint/recommended', 'prettier'],
  parser: '@typescript-eslint/parser',
  parserOptions: {
    ecmaVersion: 2022,
    sourceType: 'module',
    project: false,
    tsconfigRootDir: __dirname,
  },
  plugins: ['@typescript-eslint'],
  overrides: [
    {
      files: ['*.ts', '*.tsx'],
      rules: {
        // Keep the rules pragmatic for this scaffold; tighten as needed
        '@typescript-eslint/no-explicit-any': 'off',
        '@typescript-eslint/explicit-module-boundary-types': 'off',
        'no-console': 'off',
        // Avoid false positives with unused vars in some signatures; prefer explicit underscores
        'no-unused-vars': 'off',
        '@typescript-eslint/no-unused-vars': [
          'error',
          { argsIgnorePattern: '^_', varsIgnorePattern: '^_', caughtErrorsIgnorePattern: '^_' },
        ],
      },
    },
  ],
  ignorePatterns: ['dist/', 'node_modules/', '**/*.d.ts'],
};
