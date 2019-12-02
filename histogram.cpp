/*Jose Luis Mira Serrano
Ruben Marquez Villalta*/

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <stdio.h>
#include <omp.h>

#define COLOUR_DEPTH 4
#define VectorSize 256

double computeGraySequential(QImage *image, int histgr[]) {

  	double start_time = omp_get_wtime();
  	uchar *pixelPtr = image->bits();
  
  	for (int ii = 0; ii < image->byteCount(); ii += COLOUR_DEPTH) {
    
    		QRgb* rgbpixel = reinterpret_cast<QRgb*>(pixelPtr + ii);
    		int gray = qGray(*rgbpixel);
    		histgr[gray]++;
    		*rgbpixel = QColor(gray, gray, gray).rgba();
  	}

  	return omp_get_wtime() - start_time;  
}
/*Las esperas producidas por la seccion critica hacen que este sea el algoritmo mas lento*/
double computeGrayParallelCritical(QImage *image, int histgr[]) {
  	double start_time = omp_get_wtime();
  	uchar *pixelPtr = image->bits();
  
	#pragma omp parallel for        
  	for (int ii = 0; ii < image->byteCount(); ii += COLOUR_DEPTH) {

    		QRgb* rgbpixel = reinterpret_cast<QRgb*>(pixelPtr + ii);
    		int gray = qGray(*rgbpixel);
    		#pragma omp critical
        		histgr[gray]++;
    			*rgbpixel = QColor(gray, gray, gray).rgba();
  	}

  	return omp_get_wtime() - start_time;  
}

double computeGrayParallelAtomic(QImage *image, int histgr[]) {
  	double start_time = omp_get_wtime();
  	uchar *pixelPtr = image->bits();
  
	#pragma omp parallel for        
  		for (int ii = 0; ii < image->byteCount(); ii += COLOUR_DEPTH) {

    			QRgb* rgbpixel = reinterpret_cast<QRgb*>(pixelPtr + ii);
    			int gray = qGray(*rgbpixel);
    			#pragma omp atomic
        			histgr[gray]++;
    				*rgbpixel = QColor(gray, gray, gray).rgba();
  		}

  	return omp_get_wtime() - start_time;  
}

void createLocks(omp_lock_t lock[]) {
    	for(int i = 0; i < VectorSize; i++)
       	 	omp_init_lock(&lock[i]);
}

void deleteLocks(omp_lock_t lock[]) {
	for(int i = 0; i < VectorSize; i++)
        	omp_destroy_lock(&lock[i]);
}

double computeGrayParallelLocks(QImage *image, int histgr[]) {
  	double start_time = omp_get_wtime();
  	uchar *pixelPtr = image->bits();
  	omp_lock_t lock[VectorSize];
  	createLocks(lock);
  
	#pragma omp parallel for        
  	for (int ii = 0; ii < image->byteCount(); ii += COLOUR_DEPTH) {

    		QRgb* rgbpixel = reinterpret_cast<QRgb*>(pixelPtr + ii);
    		int gray = qGray(*rgbpixel);

    		omp_set_lock(&lock[gray]);
    		histgr[gray]++;
    		omp_unset_lock(&lock[gray]);

    		*rgbpixel = QColor(gray, gray, gray).rgba();
  	}
  	deleteLocks(lock);

  	return omp_get_wtime() - start_time;  
}

/*Algoritmo mas eficiente*/
double computeGrayParallelReduction(QImage *image, int histgr[]){
	double start_time=omp_get_wtime();
	uchar *pixelPtr = image->bits();
	#pragma omp parallel for reduction(+:histgr[:VectorSize]) /*Esta sintaxis del reduction solo puede ser utilizada con OpenMP 4.5 incluido en el compilador gcc 6.1 o superiores*/
		for(int ii=0; ii<image->byteCount(); ii+=COLOUR_DEPTH){
			QRgb* rgbpixel=reinterpret_cast<QRgb*>(pixelPtr + ii);
			int gray=qGray(*rgbpixel);
			histgr[gray]++;
			*rgbpixel=QColor(gray, gray, gray).rgba();
		}
	return omp_get_wtime() - start_time;

}

void printHistogram(int histogram[])
{
    	int i,j,x,reductionFactor=1000;
    	for(i=0; i<VectorSize; i++){
        	printf("RGB:%d || Nº Pixels:%d ",i,histogram[i]);
        	x=histogram[i]/reductionFactor;
        	for(j=0; j<=x; j++) printf("*");
        	printf("\n");
    	}
}

bool equalHistogram(int histogram1[], int histogram2[]) {
    	for(int i = 0; i < VectorSize; i++) 
        	if(histogram1[i] != histogram2[i])
                	return false;
    	return true;
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
    	int histgr[VectorSize];
   	memset(histgr, 0, sizeof(histgr));
    
    	QImage seqImage(image);
    	double computeTime = computeGraySequential(&seqImage, histgr);
    	printf("sequential time: %0.9f seconds\n", computeTime);

    	QImage auxImage(image);
    	int auxHistgr[VectorSize];
    	memset(auxHistgr, 0, sizeof(auxHistgr));
    	computeTime = computeGrayParallelCritical(&auxImage, auxHistgr);
    	printf("parallel-critical time: %0.9f seconds\n", computeTime);
    
	if (auxImage == seqImage) printf("Algoritmo secuencial y paralelo-critical dan la misma imagen\n");
	else printf("Algoritmo secuencial y paralelo dan distinta imagen\n"); 
	if (equalHistogram(histgr, auxHistgr)) printf("Algoritmo secuencial y paralelo-critical dan el mismo histograma\n");
	else printf("Algoritmo secuencial y paralelo dan distinto histograma\n");

    	auxImage = image;
    	memset(auxHistgr, 0, sizeof(auxHistgr));
    	computeTime = computeGrayParallelAtomic(&auxImage, auxHistgr);
    	printf("parallel-atomic time: %0.9f seconds\n", computeTime);
    
	if (auxImage == seqImage) printf("Algoritmo secuencial y paralelo-atomic dan la misma imagen\n");
	else printf("Algoritmo secuencial y paralelo-atomic dan distinta imagen\n");
	if (equalHistogram(histgr, auxHistgr)) printf("Algoritmo secuencial y paralelo-atomic dan el mismo histograma\n");
	else printf("Algoritmo secuencial y paralelo-atomic dan distinto histograma\n");

    	auxImage = image;
    	memset(auxHistgr, 0, sizeof(auxHistgr));
    	computeTime = computeGrayParallelLocks(&auxImage, auxHistgr);
    	printf("parallel-locks time: %0.9f seconds\n", computeTime);
    
	if (auxImage == seqImage) printf("Algoritmo secuencial y paralelo-locks dan la misma imagen\n");
	else printf("Algoritmo secuencial y paralelo-locks dan distinta imagen\n");
	if (equalHistogram(histgr, auxHistgr)) printf("Algoritmo secuencial y paralelo-locks dan el mismo histograma\n");
	else printf("Algoritmo secuencial y paralelo-locks dan distinto histograma\n");     

	
	auxImage = image;
    	memset(auxHistgr, 0, sizeof(auxHistgr));
    	computeTime = computeGrayParallelReduction(&auxImage, auxHistgr);
    	printf("parallel-reduction time: %0.9f seconds\n", computeTime);
    
	if (auxImage == seqImage) printf("Algoritmo secuencial y paralelo-reduction dan la misma imagen\n");
	else printf("Algoritmo secuencial y paralelo-reduction dan distinta imagen\n");
	if (equalHistogram(histgr, auxHistgr)) printf("Algoritmo secuencial y paralelo-reduction dan el mismo histograma\n");
	else printf("Algoritmo secuencial y paralelo-reduction dan distinto histograma\n"); 

    	printHistogram(auxHistgr);    
    	QPixmap pixmap = pixmap.fromImage(auxImage);
    	QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
    	scene.addItem(item);
           
    	view.show();
    	return a.exec();
}

