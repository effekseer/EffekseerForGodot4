# 更新履歴

## 1.70e.2
- godot-cppを4.1にアップデート (Godot 4.1未満では動かなくなりました)

## 1.70e.1
- プロジェクト設定のデフォルト値を変更
  - Instance Max Count: 2000 -> 4000
  - Square Max Count: 8000 -> 16000
  - Draw Max Count: 128 -> 256
- エフェクトロード処理を改善
- 新しいシェーダ生成処理に移行
- エディタビューポートでのプレビュー機能を追加
- "res://"直下にリソースを置くとロード失敗する不具合を修正
- テクスチャの外側モードにクランプを指定してもリピートになる不具合を修正
- Effekseerを1.70eにアップデート
- godot-cppを4.0.3にアップデート

## 1.70b.1
- 終了時のクラッシュを回避
- 3Dのトランスフォーム計算のミスを修正
- EffekseerServer.gdを削除
- EffekseerSystemに次のメソッドを追加
  - spawn_effect_2d(effect, parent, xform)
  - spawn_effect_3d(effect, parent, xform)

## 1.70b.beta2
- モデルのインスタンスドレンダリングに対応
- 高度シェーダー(高度描画パネルの機能)に対応
- Emitter3Dのギズモ表示を有効化
- シェーダプリローダーを有効化

## 1.70b.beta1
- EffekseerForGodot4最初のリリース
