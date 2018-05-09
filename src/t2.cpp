#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <string>
#include <sstream>

#define pixel(i,x,y) (i[(y)*ii.width+(x)]) 

using namespace std;

typedef struct ImageInfo{
    int width;
    int height;
}ImageInfo;

typedef struct PixelF{//Para cálculos com ponto flutuante
    float R, G, B;
} PixelF;

typedef struct Pixel
{
    uint8_t R, G, B;
}Pixel;

typedef Pixel* Imagem;

void extractHeader(ifstream *, ImageInfo*);
void writeHeader(ofstream *, ImageInfo);
void initData(Imagem *, ImageInfo);
void readData(ifstream *, Imagem, ImageInfo);
void writeData(ofstream *, Imagem, ImageInfo);

void resize(Imagem, Imagem*, ImageInfo, ImageInfo, float);
void populateCintMatrix(Pixel[4][4], Imagem, ImageInfo, float, float);
Pixel interpolate(Pixel, Pixel, Pixel, Pixel, float, int);
PixelF operator*(Pixel, float);
PixelF operator+(PixelF, PixelF);
Pixel toPixel(PixelF);

int main(int argc, char **argv){
    ifstream *fInput = new ifstream();
    ofstream *fOutput = new ofstream();
    ImageInfo iiIn, iiOut;
    Imagem dataIn, dataOut;
    float scale;

    if(argc < 4) {
        cout << "Uso: " << argv[0] << " <imagem_entrada.ppm> <imagem_saida.ppm> <escala>" << endl;
        return -1;
    }

    scale = atof(argv[3]);

    fInput->open(argv[1]);
    fOutput->open(argv[2]);

    extractHeader(fInput, &iiIn);
    initData(&dataIn, iiIn);
    readData(fInput, dataIn, iiIn);
    iiOut.width = iiIn.width * scale;
    iiOut.height = iiIn.height * scale;
    initData(&dataOut, iiOut);
    resize(dataIn, &dataOut, iiIn, iiOut, scale);
    writeHeader(fOutput, iiOut);
    writeData(fOutput, dataOut, iiOut);

    fInput->close();
    fOutput->close();
    delete fInput;
    delete fOutput;
    delete [] dataIn;
    delete [] dataOut;
}

//Le cabecalho do formato .ppm
//Recebe arquivo de entrada e escreve em ii largura e altura da imagem
void extractHeader(ifstream* f, ImageInfo* ii){
    vector<string> vec;

    for(int i = 0; i < 4; i++){
        string tok;
        *f >> tok;
        if(tok[0] == '#') {
            getline(*f, tok);
            i--;
            continue;
        }
        vec.push_back(tok);
    }
    char c;
    f->read(&c, 1);
    ii->width = stoi(vec[1]);
    ii->height = stoi(vec[2]);
}

//Escreve cabecalho do formato .ppm
//Recebe arquivo de saida e le de ii largura e altura da imagem
void writeHeader(ofstream* f, ImageInfo ii){
    *f << "P6 " << ii.width << " " << ii.height << " " << 255 << " ";
}

//Inicializa uma matriz de tamanho ii.height*ii.width em data
void initData(Imagem* data, ImageInfo ii){
    *data = new Pixel[ii.height*ii.width];
}

//Le pixels do arquivo f e escreve na matriz data
void readData(ifstream* f, Imagem data, ImageInfo ii){
    char pixel_data[3];
    for(int i = 0; i < ii.height; i++){
        for(int j = 0; j < ii.width; j++){
            f->read(pixel_data, 3);
            pixel(data,j,i).R = (uint8_t) pixel_data[0];
            pixel(data,j,i).G = (uint8_t) pixel_data[1];
            pixel(data,j,i).B = (uint8_t) pixel_data[2];
        }
    }
}

//Le pixels da matriz data e escreve no arquivo f
void writeData(ofstream* f, Imagem data, ImageInfo ii){
    char pixel_data[3];
    for(int i = 0; i < ii.height; i++){
        for(int j = 0; j < ii.width; j++){
            pixel_data[0] = (char) pixel(data,j,i).R;
            pixel_data[1] = (char) pixel(data,j,i).G;
            pixel_data[2] = (char) pixel(data,j,i).B;
            f->write(pixel_data, 3);
        }
    }
}

//Para cada pixel na imagem ampliada, pega 16 pontos ao redor e interpola como explicado
//no relatorio
void resize(Imagem in, Imagem* out, ImageInfo iiIn, ImageInfo iiOut, float scale){
    for(int i = 0; i < iiOut.height; i++){
        for(int j = 0; j < iiOut.width; j++){
            Pixel nData[4][4], c_int[4];
            float posX = j / scale;
            float posY = i / scale;
            if((posX < 1 || posX > iiIn.width - 2) || (posY < 1 || posY > iiIn.height - 2)) continue;
            populateCintMatrix(nData, in, iiIn, posX, posY);
           for(int k = 0; k < 4; k++){
                c_int[k] = interpolate( nData[k][0],nData[k][1],nData[k][2],nData[k][3], 
                                        posX, (posX < iiIn.width / 2));
            }
            (*out)[i*iiOut.width+j] = interpolate(  c_int[0],c_int[1],c_int[2],c_int[3], 
                                                    posY, (posY < iiIn.height / 2));
        }
    }
}

//Pega 16 pontos em in ao redor de posX e posY e escreve em nData
void populateCintMatrix(Pixel nData[4][4], Imagem in, ImageInfo ii, float posX, float posY){
    for (int i = 0; i < 4; i++){
        int y = (posY < ii.height / 2) ? trunc(posY)-1 : ceil(posY)-2;
        y += i;
        for (int j = 0; j < 4; j++)
        {
            int x = (posX < ii.width / 2) ? trunc(posX)-1 : ceil(posX)-2;
            x += j;
            nData[i][j] = pixel(in, x, y);
        }
    }
}

Pixel interpolate(Pixel d0, Pixel d1, Pixel d2, Pixel d3, float pos, int lower){
    float x = pos - trunc(pos);
    x += lower ? 1 : 2;
    float l0 = ((x - 1) / (0 - 1.0))*((x - 2) / (0 - 2.0))*((x - 3) / (0 - 3.0));
    float l1 = ((x - 0) / (1.0 - 0))*((x - 2) / (1.0 - 2))*((x - 3) / (1.0 - 3));
    float l2 = ((x - 0) / (2.0 - 0))*((x - 1) / (2.0 - 1))*((x - 3) / (2.0 - 3));
    float l3 = ((x - 0) / (3.0 - 0))*((x - 1) / (3.0 - 1))*((x - 2) / (3.0 - 2));
    Pixel l = toPixel((d0 * l0) + (d1 * l1) + (d2 * l2) + (d3 * l3));
    return l;
}

//Multiplicacao de Pixel por float, retorna PixelF
PixelF operator*(Pixel o, float f){
    PixelF p;
    float R, G, B;
    p.R = (o.R * f);
    p.G = (o.G * f);
    p.B = (o.B * f);
    return p;
}

//Soma de PixelF com PixelF
PixelF operator+(PixelF o, PixelF p){
    PixelF q;
    int R, G, B;
    q.R = o.R + p.R;
    q.G = o.G + p.G;
    q.B = o.B + p.B;
    return q;
}

//Converte PixelF para Pixel, observando restricoes do uint8_t (byte)
Pixel toPixel(PixelF f){
    Pixel p;
    p.R = (uint8_t)((f.R > 255) ? 255 : ((f.R<0)?0:f.R));
    p.G = (uint8_t)((f.G > 255) ? 255 : ((f.G<0)?0:f.G));
    p.B = (uint8_t)((f.B > 255) ? 255 : ((f.B<0)?0:f.B));
    return p;
}