<!-- Copyright [2023] [MicrogreensBR]

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. -->

# MicrogreensFW

Repositório principal do firmware do projeto Microgreens da disciplina Projeto em Internet das Coisas (SEL0373).

## Arquitetura e operação básica do sistema

![Arquitetura do sistema](./imgs/architecture.drawio.png)

### Fluxo do processo

1. Usuário acessa a plataforma e seleciona, de uma lista pré-definida, a planta que deseja-se cultivar. A partir desta escolha, a plataforma informa o usuário sobre cuidados básicos com a planta.
2. Usuário planta as sementes, coloca bandeija sobre estas e informa a plataforma que o processo foi iniciado.
3. Plataforma envia ao ESP32 os dados sobre o cultivo da planta.
4. Quando o processo for finalizado, o ESP32 informa a platafora.
5. A plataforma informa o usuário de que o processo foi finalizado.
6. Usuário colhe a planta.

Caso seja identificado algum problema, o usuário é informado.

### Processos monitorados e controlados pelo ESP32

- Temperatura (monitorar por sensor e controlar por ventilação)
- Umidade do solo (monitorar por sensor e controlar por irrigação)
- Altura da planta (monitoriar por sensor ToF)
- Iluminação (controlar por LED)

## Placa utilizada

Confidencial.

## Materiais

A lista de materiais (exceto pela placa) pode ser visualizada [nesta planilha](https://docs.google.com/spreadsheets/d/1QAQEL1R4-6mjEcpoBJlASzv0y4lyy3VPI9GnsOz3ivg/edit#gid=0).

## Colaboradores

- Henrique Megid
- [Henrique Sander Lourenço](https://github.com/hsanderr)
- Tiago Xavier

## Licença

Copyright 2023 MicrogreensBR. Released under the [Apache 2.0 license](./LICENSE).
