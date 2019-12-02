#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <stdio.h>
#include <omp.h>

#define COLOUR_DEPTH 4

double computeGraySequential(QImage *image) {
  double start_time = omp_get_wtime();
  uchar *pixelPtr = image->bits();
  
  for (int ii = 0; ii < image->byteCount(); ii += COLOUR_DEPTH) {
    
    QRgb* rgbpixel = reinterpret_cast<QRgb*>(pixelPtr + ii);
    int gray = qGray(*rgbpixel);
    *rgbpixel = QColor(gray, gray, gray).rgba();
  }
  return omp_get_wtime() - start_time;  
}

double computeGrayParallel(QImage *image) {
  double start_time = omp_get_wtime();
  uchar *pixelPtr = image->bits();
  
#pragma omp parallel for        
  for (int ii = 0; ii < image->byteCount(); ii += COLOUR_DEPTH) {
    
    QRgb* rgbpixel = reinterpret_cast<QRgb*>(pixelPtr + ii);
    int gray = qGray(*rgbpixel);
    *rgbpixel = QColor(gray, gray, gray).rgba();
  }
  return omp_get_wtime() - start_time;  
}

double computeGrayScanline(QImage *image) {
  double start_time = omp_get_wtime();
  int alto = image->height(); int ancho = image->width();
  int jj, gray; uchar* scan; QRgb* rgbpixel;  
  for (int ii = 0; ii < alto; ii++) {
    
    scan = image->scanLine(ii);
    for (jj = 0; jj < ancho; jj++) {
      
      rgbpixel = reinterpret_cast<QRgb*>(scan + jj * COLOUR_DEPTH);
      gray = qGray(*rgbpixel);
      *rgbpixel = QColor(gray, gray, gray).rgba();
    }
  }
  return omp_get_wtime() - start_time;    
}



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QPixmap qp = QPixmap("test_1080p.bmp"); // ("c:\\test_1080p.bmp");
    if(qp.isNull())
    {
        printf("image not found\n");
		return -1;
    }
    
    QImage image = qp.toImage();
    
    QImage seqImage(image);
    double computeTime = computeGraySequential(&seqImage);
    printf("sequential time: %0.9f seconds\n", computeTime);

    QImage auxImage(image);
    computeTime = computeGrayParallel(&auxImage);
    printf("parallel time: %0.9f seconds\n", computeTime);
    
	if (auxImage == seqImage) printf("Algoritmo secuencial y paralelo dan la misma imagen\n");
	else printf("Algoritmo secuencial y paralelo dan distinta imagen\n");    

    auxImage = image;
    computeTime = computeGrayScanline(&auxImage);
    printf("scanline time: %0.9f seconds\n", computeTime);
    
	if (auxImage == seqImage) printf("Algoritmo básico y 'scanline' dan la misma imagen\n");
	else printf("Algoritmo básico y 'scanline' dan distinta imagen\n");    

        
    QPixmap pixmap = pixmap.fromImage(auxImage);
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
    scene.addItem(item);
           
    view.show();
    return a.exec();
}
