<div>
    <img src="./img\embarcatech_logo.png" alt="Logo Embarcatech" height="90">
    <img align="right" src="./img\cepedi_logo.png" alt="Logo Embarcatech" height="90">
</div>

<h2 align="center"> Ohmímetro na BitDogLab </h2>
<h5 align="center"> Atividade promovida pela Residência (fase 2) do programa EmbarcaTech. </h5>

<br>

## Sobre o projeto
O projeto simula um ohmímetro, identificando o valor do resistor medido, além de exibir o código de cores correspondente no display OLED (SSD1306) da placa BitDogLab. A descrição completa do projeto pode ser encontrada na [Ficha de proposta de projeto](docs\TrabalhoSE_FSA_1_Naylane_Ribeiro.pdf).

<br>

## Tecnologias e Ferramentas utilizadas
- **Kit BitDogLab**
- **Jumpers**: MxM e MxF
- **Resistores**: 1/4W
- **Linguagem de programação:** C
- **Ambiente de Desenvolvimento:** Visual Studio Code

<br>

## Instruções de uso
- É necessário instalar o SDK da Raspberry Pi Pico e realizar as configurações iniciais.

1. Para ter acesso ao projeto, **clone o repositório**:
```bash
git clone https://github.com/naylane/Ohmimetro-BitDogLab.git
```
2. Certifique de configurar o projeto, checando o ```CMakeLists.txt``` e o SDK.
3. Compile o projeto pela extensão da Raspberry Pi Pico ou terminal. Opção pelo terminal abaixo:
```bash
mkdir build && cd build
cmake ..
make 
```
4. Coloque a BitDogLab em modo BOOTSEL (pressione o botão BOOTSEL ao conectar o cabo USB).

<br>

## Desenvolvedora
<table>
  <tr>
    <td align="center"><img style="" src="https://avatars.githubusercontent.com/u/89545660?v=4" width="100px;" ><br /> <sub> <b> Naylane Ribeiro </b> </sub>
    </td>
</table>

<img width=100% src="https://capsule-render.vercel.app/api?type=waving&color=2c9ad6&height=120&section=footer"/>
