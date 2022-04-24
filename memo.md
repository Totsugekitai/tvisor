# Tips

- `vmxon` の処理は `on_each_cpu` の `wait` 引数を `true` にして同期を取らないとなんか失敗するので注意