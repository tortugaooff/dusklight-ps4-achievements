# Dusklight PS4 r25 — Conquistas offline

Variante experimental independente para testar três conquistas próprias no PS4 Slim.

## Créditos do código-base

- zeldaret e colaboradores pela decompilação de Twilight Princess.
- Twilit Realm e colaboradores pelo Dusklight e por Aurora e Borealis.
- mshivam019 pelo port PS5 que serve de base ao r25.
- BgbrhrWatchinOVO e colaboradores pelo port Dusklight PS4 r25.
- iHaiDeeZ e colaboradores do DolphinPS4 pela infraestrutura de plataforma e Mesa sobre GNM.
- OTPR26 pelo Twilight HD HUD. OpenOrbis e colaboradores de LibOrbisPkg, GoldHEN, Mesa, SDL, Dawn, LLVM e demais projetos pelas ferramentas e bibliotecas.
- Créditos adicionais e termos de cada componente constam em [THIRD_PARTY_LICENSES.txt](THIRD_PARTY_LICENSES.txt).

## Modificações desta variante (2026-11-21)

O patch desta variante acrescenta três conquistas offline, registro separado, menu e notificações em português. Também isola a raiz de dados de teste. O trabalho parte do Dusklight PS5 no commit 5811958531cb0a9eafeb293fafcb36071883bfdf e das modificações do r25, sem atribuir essas bases ao projeto de conquistas.

## Licença e fonte correspondente

O port PS4 combinado é distribuído sob [GPL-3.0](LICENSE). Os componentes preservam seus avisos e termos próprios. A fonte correspondente pode ser reconstruída com o patch, o workflow e os scripts deste repositório no commit da execução, além do código e patches das dependências públicas indicadas pelo bootstrap. O artefato inclui as referências de origem e os avisos. Este projeto independente não é afiliado à Nintendo, Sony ou Twilit Realm.

## Build e teste

O Actions usa um runner público padrão. O artefato contém apenas o executável, o preset, o relatório ELF, os hashes, as referências de fonte e os avisos. O empacotamento local usa a árvore de recursos do próprio port e as cópias locais do disco e da tradução. Conferir que DSKL00002 está livre antes de instalar, e preservar o DSKL00001 e sua raiz de dados.
