/**
 * MMS25-26 Praktikumsaufgaben
 * Implementierung aller Funktionen für Signalverarbeitung und -analyse
 */

#include "MMS25_26.h"
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// INTERNE HILFSFUNKTIONEN (nicht im Header deklariert)
// ============================================================================
/**
 * Vergleichsfunktion für qsort() zum Sortieren von double-Werten
 * Wird für Median-Berechnung benötigt
 */
static int compareDoubles(const void *a, const void *b) {
    double x = *(const double*)a;
    double y = *(const double*)b;
    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

/**
 * Zählt die Anzahl gültiger Zahlenzeilen in einer Datei
 * Wird für createSignal_file() verwendet
 */
static int countLinesInFileHelper(char *fileName) {
    FILE *fp = fopen(fileName, "r");
    if (!fp) return 0;

    int lines = 0;
    char buf[4096];
    while (fgets(buf, sizeof(buf), fp)) {
        double tmp;
        if (sscanf(buf, "%lf", &tmp) == 1) lines++;  // Nur Zeilen mit gültiger Zahl zählen
    }
    fclose(fp);
    return lines;
}

/**
 * Berechnet die Varianz eines Signals
 * Varianz = Durchschnitt der quadrierten Abweichungen vom Mittelwert
 * Wird intern für Standardabweichung verwendet
 */
static double computeVariance_internal(MMSignal *In) {
    if (!In || !In->samples || In->numberOfSamples <= 0) return 0.0;
    
    // Mittelwert berechnen
    double sum = 0.0;
    for (int i = 0; i < In->numberOfSamples; i++) sum += In->samples[i];
    double mean = sum / (double)In->numberOfSamples;
    
    // Summe der quadrierten Abweichungen
    double sumSq = 0.0;
    for (int i = 0; i < In->numberOfSamples; i++) {
        double d = In->samples[i] - mean;
        sumSq += d * d;
    }
    return sumSq / (double)In->numberOfSamples;
}

// ============================================================================
// AUFGABE 1: IMPLEMENTIERUNG
// ============================================================================

/**
 * Aufgabe 1.1: Lineare Interpolation zwischen zwei Punkten
 * Berechnet den y-Wert an der Stelle xb zwischen (x1,y1) und (x2,y2)
 * Formel: y = y1 + (xb - x1) * (y2 - y1) / (x2 - x1)
 */
double interpolateLine(double x1, double y1, double x2, double y2, double xb) {
    if (x2 == x1) {
        return y1; // Vertikale Linie: Division durch 0 vermeiden
    }
    double slope = (y2 - y1) / (x2 - x1);  // Steigung berechnen
    return y1 + (xb - x1) * slope;         // Interpolierter Wert
}

/**
 * Aufgabe 1.2: Skaliert Werte auf neues Minimum mit Skalierungsfaktor
 * 1. Findet aktuelles Minimum
 * 2. Verschiebt alle Werte so, dass Minimum bei 0 liegt
 * 3. Multipliziert mit Skalierungsfaktor
 * 4. Verschiebt auf neues Zielminimum
 */
double *scaleValuesInArray(int numberOfValues, double *values, double minimum, double scalingFactor) {
    if (numberOfValues <= 0 || !values) return NULL;

    // Aktuelles Minimum finden
    double curMin = DBL_MAX;
    for (int i = 0; i < numberOfValues; i++) {
        if (values[i] < curMin) curMin = values[i];
    }
    
    // Werte transformieren: (Wert - altMin) * Faktor + neuMin
    for (int i = 0; i < numberOfValues; i++) {
        values[i] = (values[i] - curMin) * scalingFactor + minimum;
    }
    return values;
}

/**
 * Aufgabe 1.3: Erzeugt Sinus-Schwingung
 * @param totalSamples: Gesamtanzahl der zu erzeugenden Samples
 * @param samplesPerPeriod: Anzahl Samples pro Periode (bestimmt Frequenz)
 * @param amplitude: Amplitude der Schwingung
 * @return: Array mit Sinus-Werten oder NULL bei Fehler
 */
double *createSineArray(int totalSamples, int samplesPerPeriod, double amplitude) {
    if (totalSamples <= 0 || samplesPerPeriod <= 0 || amplitude < 0) return NULL;

    double *arr = (double*)malloc((size_t)totalSamples * sizeof(double));
    if (!arr) return NULL;

    const double twoPi = 2.0 * M_PI;
    // Berechne Sinus-Werte: A * sin(2π * i / samplesPerPeriod)
    for (int i = 0; i < totalSamples; i++) {
        arr[i] = amplitude * sin(twoPi * (double)i / (double)samplesPerPeriod);
    }
    return arr;
}

/**
 * Aufgabe 1.4: Export eines Arrays in Datei (ASCII, eine Zahl pro Zeile)
 * @return: Anzahl erfolgreich geschriebener Werte
 */
int writeArrayFile(char *filePath, double *array, int arrayLength) {
    if (!filePath || !array || arrayLength <= 0) return 0;
    FILE *fp = fopen(filePath, "w");
    if (!fp) return 0;

    int written = 0;
    // Schreibe jeden Wert in eine neue Zeile
    for (int i = 0; i < arrayLength; i++) {
        if (fprintf(fp, "%lf\n", array[i]) >= 0) written++;
    }
    fclose(fp);
    return written;
}

/**
 * Aufgabe 1.5: Liest Zahlen aus Datei ein
 * @param values: Voralloziertes Array (Größe muss ausreichend sein!)
 * @return: Anzahl gelesener Werte oder -1 bei Fehler
 */
/*
int readArrayFile(char *fileName, double *values) {
    if (!fileName || !values) return -1;

    FILE *fp = fopen(fileName, "r");
    if (!fp) return -1;

    int count = 0;
    char buf[4096];
    // Lese Zeile für Zeile und parse double-Werte
    while (fgets(buf, sizeof(buf), fp)) {
        if (sscanf(buf, "%lf", &values[count]) == 1) count++;
    }
    fclose(fp);
    return count;
}
*/

double *readArrayFile(char *fileName, int *arrayLength) {
    if (!fileName || !arrayLength) return NULL;

    *arrayLength = 0;

    int n = countLinesInFileHelper(fileName);
    if (n <= 0) return NULL;

    double *values = (double*)malloc((size_t)n * sizeof(double));
    if (!values) return NULL;

    FILE *fp = fopen(fileName, "r");
    if (!fp) {
        free(values);
        return NULL;
    }

    int count = 0;
    char buf[4096];
    while (fgets(buf, sizeof(buf), fp) && count < n) {
        double temp;

        if (sscanf(buf, "%lf", &temp) == 1) {
            values [count] = temp;
            count++;
        }
    }
    fclose(fp);
    *arrayLength = count;
    return values;
}

MMSignal *createSignal_array(int numberOfValues, double *values) {
    if (numberOfValues <= 0) return NULL;

    MMSignal *sig = (MMSignal*)malloc(sizeof(MMSignal));
    if (!sig) return NULL;

    sig->numberOfSamples = numberOfValues;
    sig->samples = (double*)malloc((size_t)numberOfValues * sizeof(double));
    if (!sig->samples) { free(sig); return NULL; }

    if (values) memcpy(sig->samples, values, (size_t)numberOfValues * sizeof(double));
    else memset(sig->samples, 0, (size_t)numberOfValues * sizeof(double));

    sig->area = 0.0;
    sig->mean = 0.0;
    sig->localExtrema = NULL;

    return sig;
}

/*
MMSignal *createSignal_file(char *fileName) {
    if (!fileName) return NULL;

    int n = countLinesInFileHelper(fileName);
    if (n <= 0) return NULL;

    MMSignal *sig = createSignal_array(n, NULL);
    if (!sig) return NULL;

    int readCount = readArrayFile(fileName, sig->samples);
    if (readCount != n) {
        deleteMMSignal(sig);
        return NULL;
    }
    return sig;
}
*/

MMSignal *createSignal_file(char *fileName) {
    if (!fileName) return NULL;

    int length = 0;

    double *temporaryData = readArrayFile(fileName, &length);
    if (!temporaryData || length <= 0) {
        if (temporaryData) free(temporaryData);
        return NULL;
    }

    MMSignal *sig = createSignal_array(length, temporaryData);
    free(temporaryData); // free the temporary data array before returning
    return sig;

}

void writeSignal(MMSignal *In, char * fileName) {
    if (!In || !fileName) return;
    writeArrayFile(fileName, In->samples, In->numberOfSamples);
}

void deleteMMSignal(MMSignal *In) {
    if (!In) return;

    if (In->samples) { free(In->samples); In->samples = NULL; }

    if (In->localExtrema) {
        if (In->localExtrema->minimumPositionArray) {
            free(In->localExtrema->minimumPositionArray);
            In->localExtrema->minimumPositionArray = NULL;
        }
        if (In->localExtrema->maximumPositionArray) {
            free(In->localExtrema->maximumPositionArray);
            In->localExtrema->maximumPositionArray = NULL;
        }
        free(In->localExtrema);
        In->localExtrema = NULL;
    }
    free(In);
}

MMSignal *createSineSignal(int totalSamples, int samplesPerPeriod, double amplitude) {
    double *arr = createSineArray(totalSamples, samplesPerPeriod, amplitude);
    if (!arr) return NULL;
    MMSignal *sig = createSignal_array(totalSamples, arr);
    free(arr);
    return sig;
}

// ============================================================================
// AUFGABE 2: IMPLEMENTIERUNG - STATISTISCHE BERECHNUNGEN
// ============================================================================

/**
 * Aufgabe 2.1: Findet lokale Extrema (Minima und Maxima)
 * Ein lokales Maximum liegt vor, wenn: samples[i] > samples[i-1] UND samples[i] > samples[i+1]
 * Ein lokales Minimum liegt vor, wenn: samples[i] < samples[i-1] UND samples[i] < samples[i+1]
 */
LocalExtrema *computeExtrema(MMSignal *In) {
    if (!In || !In->samples) return NULL;

    if (In->localExtrema) {
        if (In->localExtrema->minimumPositionArray) free(In->localExtrema->minimumPositionArray);
        if (In->localExtrema->maximumPositionArray) free(In->localExtrema->maximumPositionArray);
        free(In->localExtrema);
        In->localExtrema = NULL;
    }

    LocalExtrema *le = (LocalExtrema*)malloc(sizeof(LocalExtrema));
    if (!le) return NULL;
    le->numberOfMinimumPositions = 0;
    le->numberOfMaximumPositions = 0;
    le->minimumPositionArray = NULL;
    le->maximumPositionArray = NULL;

    if (In->numberOfSamples < 3) {
        In->localExtrema = le;
        return le;
    }

    for (int i = 1; i < In->numberOfSamples - 1; i++) {
        double p = In->samples[i-1];
        double c = In->samples[i];
        double n = In->samples[i+1];
        if (c > p && c > n) le->numberOfMaximumPositions++;
        else if (c < p && c < n) le->numberOfMinimumPositions++;
    }

    if (le->numberOfMinimumPositions > 0) {
        le->minimumPositionArray = (int*)malloc((size_t)le->numberOfMinimumPositions * sizeof(int));
        if (!le->minimumPositionArray) { free(le); return NULL; }
    }
    if (le->numberOfMaximumPositions > 0) {
        le->maximumPositionArray = (int*)malloc((size_t)le->numberOfMaximumPositions * sizeof(int));
        if (!le->maximumPositionArray) { free(le->minimumPositionArray); free(le); return NULL; }
    }

    int mi = 0, ma = 0;
    for (int i = 1; i < In->numberOfSamples - 1; i++) {
        double p = In->samples[i-1];
        double c = In->samples[i];
        double n = In->samples[i+1];
        if (c > p && c > n) le->maximumPositionArray[ma++] = i;
        else if (c < p && c < n) le->minimumPositionArray[mi++] = i;
    }

    In->localExtrema = le;
    return le;
}

/**
 * Aufgabe 2.2: Berechnet Fläche unter dem Signal
 * Fläche = Summe aller Sample-Werte
 */
double computeArea(MMSignal *In) {
    if (!In || !In->samples || In->numberOfSamples <= 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < In->numberOfSamples; i++) sum += In->samples[i];
    In->area = sum;  // Speichere Ergebnis in Struktur
    return sum;
}

/**
 * Aufgabe 2.3: Berechnet Mittelwert des Signals
 * Mittelwert = Summe aller Werte / Anzahl Werte
 */
double computeMean(MMSignal *In) {
    if (!In || !In->samples || In->numberOfSamples <= 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < In->numberOfSamples; i++) sum += In->samples[i];
    In->mean = sum / (double)In->numberOfSamples;
    return In->mean;
}

/**
 * Aufgabe 2.4: Berechnet Standardabweichung
 * Standardabweichung = Wurzel aus Varianz
 * Maß für die Streuung der Werte um den Mittelwert
 */
double computeStandardDeviation(MMSignal *In) {
    double var = computeVariance_internal(In);
    return sqrt(var);
}

/**
 * Aufgabe 2.5: Berechnet Median (mittlerer Wert)
 * 1. Sortiere alle Werte
 * 2. Bei ungerader Anzahl: nimm mittleren Wert
 * 3. Bei gerader Anzahl: Mittelwert der beiden mittleren Werte
 */
double computeMedian(MMSignal *In) {
    if (!In || !In->samples || In->numberOfSamples <= 0) return 0.0;

    int n = In->numberOfSamples;
    // Kopie erstellen, da qsort das Array verändert
    double *tmp = (double*)malloc((size_t)n * sizeof(double));
    if (!tmp) return 0.0;
    memcpy(tmp, In->samples, (size_t)n * sizeof(double));
    qsort(tmp, (size_t)n, sizeof(double), compareDoubles);

    double med;
    if (n % 2 == 1) med = tmp[n/2];                              // Ungerade: mittlerer Wert
    else med = 0.5 * (tmp[n/2 - 1] + tmp[n/2]);                 // Gerade: Durchschnitt der beiden mittleren

    free(tmp);
    return med;
}

int *getHistogram(int numberOfValues, double *values, int numberOfBins) {
    if (!values || numberOfValues <= 0 || numberOfBins <= 0) return NULL;

    int *bins = (int*)calloc((size_t)numberOfBins, sizeof(int));
    if (!bins) return NULL;

    double min = DBL_MAX, max = -DBL_MAX;
    for (int i = 0; i < numberOfValues; i++) {
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
    }
    if (max == min) {
        bins[0] = numberOfValues;
        return bins;
    }

    double width = (max - min) / (double)numberOfBins;
    for (int i = 0; i < numberOfValues; i++) {
        int idx = (int)((values[i] - min) / width);
        if (idx >= numberOfBins) idx = numberOfBins - 1;
        if (idx < 0) idx = 0;
        bins[idx]++;
    }
    return bins;
}

Histogram *createHistogram_empty() {
    Histogram *h = (Histogram*)malloc(sizeof(Histogram));
    if (!h) return NULL;
    h->numberOfBins = 0;
    h->bins = NULL;
    h->minimum = 0.0;
    h->maximum = 0.0;
    h->binWidth = 0.0;
    return h;
}

Histogram *createHistogram_bins(int numberOfBins) {
    if (numberOfBins < 0) return NULL;
    Histogram *h = createHistogram_empty();
    if (!h) return NULL;
    if (numberOfBins == 0) return h;

    h->numberOfBins = numberOfBins;
    h->bins = (int*)calloc((size_t)numberOfBins, sizeof(int));
    if (!h->bins) { free(h); return NULL; }
    return h;
}

Histogram *createHistogram_array(int numberOfValues, double *values, int numberOfBins) {
    if (!values || numberOfValues <= 0 || numberOfBins <= 0) return NULL;

    Histogram *h = createHistogram_empty();
    if (!h) return NULL;

    double min = DBL_MAX, max = -DBL_MAX;
    for (int i = 0; i < numberOfValues; i++) {
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
    }

    h->numberOfBins = numberOfBins;
    h->minimum = min;
    h->maximum = max;

    h->bins = (int*)calloc((size_t)numberOfBins, sizeof(int));
    if (!h->bins) { free(h); return NULL; }

    if (max == min) {
        h->binWidth = 0.0;
        h->bins[0] = numberOfValues;
        return h;
    }

    h->binWidth = (max - min) / (double)numberOfBins;
    for (int i = 0; i < numberOfValues; i++) {
        int idx = (int)((values[i] - min) / h->binWidth);
        if (idx >= numberOfBins) idx = numberOfBins - 1;
        if (idx < 0) idx = 0;
        h->bins[idx]++;
    }
    return h;
}

void deleteHistogram(Histogram *In) {
    if (!In) return;
    if (In->bins) free(In->bins);
    free(In);
}

/**
 * Aufgabe 2.7: Berechnet Entropie des Histogramms
 * Entropie = -Σ(p(i) * log2(p(i))) in Bits
 * Maß für die Unordnung/Information in der Verteilung
 * Hohe Entropie = gleichmäßige Verteilung, niedrige Entropie = konzentrierte Verteilung
 */
double computeEntropy(Histogram *histogramIn) {
    if (!histogramIn || !histogramIn->bins || histogramIn->numberOfBins <= 0) return 0.0;

    // Gesamtanzahl der Werte berechnen
    long total = 0;
    for (int i = 0; i < histogramIn->numberOfBins; i++) total += histogramIn->bins[i];
    if (total == 0) return 0.0;

    // Shannon-Entropie: H = -Σ(p * log2(p))
    double H = 0.0;
    for (int i = 0; i < histogramIn->numberOfBins; i++) {
        if (histogramIn->bins[i] > 0) {
            double p = (double)histogramIn->bins[i] / (double)total;  // Wahrscheinlichkeit
            H -= p * log2(p);                                         // Beitrag zur Entropie
        }
    }
    return H;
}

// ============================================================================
// AUFGABE 3: IMPLEMENTIERUNG - FALTUNG UND GAUSS-APPROXIMATION
// ============================================================================

/**
 * Aufgabe 3.1: Faltung zweier Signale (Convolution)
 * Faltung: (f * g)[i] = Σ f[k] * g[i-k]
 * Ausgabelänge = N + M - 1 (N = Länge In1, M = Länge In2)
 * Anwendung: Filterung, Glättung, Systemantwort
 */
MMSignal *convoluteSignals(MMSignal *In1, MMSignal *In2) {
    if (!In1 || !In2 || !In1->samples || !In2->samples) return NULL;

    int N = In1->numberOfSamples;
    int M = In2->numberOfSamples;
    int L = N + M - 1;

    MMSignal *out = createSignal_array(L, NULL);
    if (!out) return NULL;

    for (int i = 0; i < L; i++) {
        out->samples[i] = 0.0;
        for (int k = 0; k < N; k++) {
            int j = i - k;
            if (j >= 0 && j < M) {
                out->samples[i] += In1->samples[k] * In2->samples[j];
            }
        }
    }
    return out;
}

/**
 * Aufgabe 3.2: Approximiert Gauß-Glockenkurve über Pascal-Dreieck
 * Pascal-Dreieck Zeile n gibt normierte Binomialkoeffizienten
 * Für große n (>40) nähert sich die Form einer Gauß-Kurve
 * Berechnung: C(n,k) / 2^n = [n! / (k! * (n-k)!)] / 2^n
 */
MMSignal *approximateGaussianBellCurve(int pascalLineNumber) {
    if (pascalLineNumber < 0) return NULL;

    int num = pascalLineNumber + 1;  // Zeile n hat n+1 Einträge
    MMSignal *sig = createSignal_array(num, NULL);
    if (!sig || !sig->samples) return NULL;

    // Starte mit C(n,0) / 2^n = 1 / 2^n
    double p = pow(0.5, pascalLineNumber);
    sig->samples[0] = p;

    // Iterative Berechnung: C(n,k) = C(n,k-1) * (n-k+1) / k
    for (int k = 1; k < num; k++) {
        p = p * (double)(pascalLineNumber - k + 1) / (double)k;
        sig->samples[k] = p;
    }
    return sig;
}

// ============================================================================
// AUFGABE 4: IMPLEMENTIERUNG - DISKRETE FOURIER-TRANSFORMATION
// ============================================================================

/**
 * Aufgabe 4.1: Diskrete Fourier-Transformation (DFT)
 * 
 * Vorwärts (Direction=1): Zeit-Domain -> Frequenz-Domain
 *   X[k] = Σ(n=0 bis N-1) x[n] * e^(-i*2π*k*n/N)
 * 
 * Rückwärts (Direction=-1): Frequenz-Domain -> Zeit-Domain
 *   x[n] = (1/N) * Σ(k=0 bis N-1) X[k] * e^(i*2π*k*n/N)
 * 
 * Komplexität: O(N²) - für große N ist FFT effizienter
 */
void dft(int numberOfValues, double* realIn, double* imaginaryIn,
         double* realOut, double* imaginaryOut, int Direction) {
    if (numberOfValues <= 0 || !realIn || !realOut || !imaginaryOut) return;
    if (Direction != 1 && Direction != -1) return;

    int hasImag = (imaginaryIn != NULL);
    const double twoPiOverN = 2.0 * M_PI / (double)numberOfValues;

    // Für jede Frequenz k
    for (int k = 0; k < numberOfValues; k++) {
        double sumRe = 0.0, sumIm = 0.0;

        // Summiere über alle Zeit-Samples n
        for (int n = 0; n < numberOfValues; n++) {
            double xRe = realIn[n];
            double xIm = hasImag ? imaginaryIn[n] : 0.0;

            // e^(±i*θ) = cos(θ) ± i*sin(θ), wobei θ = 2π*k*n/N
            double angle = twoPiOverN * (double)k * (double)n;
            double c = cos(angle), s = sin(angle);

            // Komplexe Multiplikation: (a+bi)*(c±di)
            if (Direction == 1) {  // Forward: e^(-i*θ)
                sumRe += xRe * c + xIm * s;
                sumIm += xIm * c - xRe * s;
            } else {                // Backward: e^(+i*θ)
                sumRe += xRe * c - xIm * s;
                sumIm += xRe * s + xIm * c;
            }
        }

        // Normalisierung nur bei Rücktransformation
        if (Direction == -1) {
            sumRe /= (double)numberOfValues;
            sumIm /= (double)numberOfValues;
        }

        realOut[k] = sumRe;
        imaginaryOut[k] = sumIm;
    }
}

/**
 * Aufgabe 4.2: Konvertiert kartesische in polare Koordinaten
 * Kartesisch: z = a + bi (Realteil + Imaginärteil)
 * Polar: z = r * e^(iφ) (Amplitude * Phase)
 * 
 * Amplitude: r = √(a² + b²)
 * Phase (Winkel): φ = atan2(b, a) im Bereich [-π, π]
 */
void getCartesianToPolar(int numberOfValues, double* realIn, double* imaginaryIn,
                         double* amplitudesOut, double* angelsOut) {
    if (numberOfValues <= 0 || !realIn || !imaginaryIn || !amplitudesOut || !angelsOut) return;

    for (int i = 0; i < numberOfValues; i++) {
        double re = realIn[i];
        double im = imaginaryIn[i];
        amplitudesOut[i] = sqrt(re*re + im*im);  // Betrag/Amplitude
        angelsOut[i] = atan2(im, re);             // Phase/Winkel
    }
}

/**
 * Aufgabe 4.2: Konvertiert polare in kartesische Koordinaten
 * Polar: z = r * e^(iφ) (Amplitude * Phase)
 * Kartesisch: z = a + bi (Realteil + Imaginärteil)
 * 
 * Realteil: a = r * cos(φ)
 * Imaginärteil: b = r * sin(φ)
 * 
 * Euler-Formel: e^(iφ) = cos(φ) + i*sin(φ)
 */
void getPolarToCartesian(int numberOfValues, double* amplitudesIn, double* angelsIn,
                         double* realOut, double* imaginaryOut) {
    if (numberOfValues <= 0 || !amplitudesIn || !angelsIn || !realOut || !imaginaryOut) return;

    for (int i = 0; i < numberOfValues; i++) {
        double A = amplitudesIn[i];  // Amplitude
        double a = angelsIn[i];      // Phase/Winkel
        realOut[i] = A * cos(a);     // Realteil
        imaginaryOut[i] = A * sin(a); // Imaginärteil
    }
}
