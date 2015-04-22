#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QList>
#include <QDir>
#include <QFile>
#include <QHash>
#include <iostream>
#include "meminfo.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //ABA PROCESSOS
    model = new QStandardItemModel();
    QStringList headers;
    headers << tr("Nome") << tr("Status") << tr("PID") << tr("PPID") << tr("Usuário") << tr("Threads") << tr("Trocas de Contexto");
    model->setHorizontalHeaderLabels(headers);

    procs = new Processos();

    connect(procs, SIGNAL(processInfo(QHash <QString, QString>)), SLOT (updateProcesses(QHash<QString, QString>)));
    procs->run();

    ui->Nprocs->display(procs->getNumProcessos());
    ui->NThreads->display(procs->getNumThreads());

    //ABA DESEMPENHO

    x.resize(60);
    for (int i = 60, j = 0; i > 0; i--, j++)
        x[j] = j;

    //GRÁFICO DA CPU

    setCPUgraph();
    threadCPU = new CPUinfo();
    threadCPU->start();

    qRegisterMetaType<QVector<double> >("QVector<double>");
    connect(threadCPU, SIGNAL(update(QVector<double>)), SLOT(updateCPU(QVector<double>)));

    //GRÁFICO DA MEMÓRIA

    memData.resize(60);
    memData.fill(0);
    threadMem = new MEMinfo();
    setMemoryGraph();

    //connect(threadMem, SIGNAL(update(QVector<double>, QVector<double>)), SLOT(updateMemoryGraph(QVector<double>, QVector<double>)));
    connect(threadMem, SIGNAL(update(double)), SLOT(updateMemoryGraph(double)));
    threadMem->start();
    //threadMem->run();

    //ABA INFORMAÇÕES DO SISTEMA

    //ICONE
    QPixmap icone("pinguim.jpg");
    ui->iconeSistema->setFixedWidth(64);
    ui->iconeSistema->setFixedHeight(64);
    ui->iconeSistema->setPixmap(icone);
    //ui->iconeSistema->show();

    //NOME DO COMPUTADOR
    QFile file("/proc/sys/kernel/hostname");
    QStringList fileData, attrib;
    QString HWinfo;
    QHash<QString, QString> hash;
    QString key, value, aux("Nome do Computador: ");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
        aux += file.readLine();
        ui->NomePC->setText(aux);
    }
    file.close();

    //VERSÃO
    file.setFileName("/proc/version_signature");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
        aux = file.readLine();
        ui->infoSO->setText(aux);
        ui->infoSO->adjustSize();
        //AJUSTAR QUEBRA DE LINHA
    }
    file.close();


    //INFORMAÇÕES DO COMPUTADOR
    file.setFileName("/proc/cpuinfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
        aux = file.readAll();
        fileData = aux.split("\n");
        for (int j = 0; j < 5; j++){
            attrib = fileData.at(j).split(":"); //SEPARANDO IDENTIFICADOR E DADO
            key = attrib.at(0).simplified();
            value = attrib.at(1).simplified();
            hash.insert(key, value);
        }
        HWinfo = hash.value("model name");
        HWinfo += "\n";
    }
    file.close();

   //INFORMAÇÕES DE MEMORIA
    double memSize;
    memSize = (threadMem->getTotal())/(1024*1024);
    HWinfo += "Memoria: " + value.setNum(memSize, 'f', 1) + " GB (";
    memSize = (threadMem->getFree())/(1024*1024);
    HWinfo += value.setNum(memSize, 'f', 1) + " GB livres )";
    ui->infoHW->setText(HWinfo);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::setMemoryGraph(){
    ui->memoryGraph->yAxis->setRange(0, 100);
    ui->memoryGraph->yAxis->setAutoTickStep(false);
    ui->memoryGraph->yAxis->setTickStep(50);
    ui->memoryGraph->xAxis->setRange(0, 60);
    ui->memoryGraph->xAxis->setTickStep(15);
    ui->memoryGraph->addGraph();
    ui->memoryGraph->graph(0)->setName("Uso de Memória");
    //ui->memoryGraph->legend->setVisible(true);
    //ui->memoryGraph->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignLeft|Qt::AlignBottom);


}

void MainWindow::updateMemoryGraph(double latestData){
    double value;
    for (int pos = 0; pos < 59; ++pos){
        value = memData[pos+1];
        memData.replace(pos, value);
    }
    memData[59] = latestData;
    ui->memoryGraph->graph(0)->setData(x, memData);
    ui->memoryGraph->replot();
}

void MainWindow::updateProcesses(QHash<QString, QString> newHash){
   listaItem.clear();
   listaItem << new QStandardItem(newHash.value("Name"));
   listaItem << new QStandardItem(newHash.value("State"));
   listaItem << new QStandardItem(newHash.value("Pid"));
   listaItem << new QStandardItem(newHash.value("PPid"));
   listaItem << new QStandardItem(newHash.value("Uid"));   // CONVERTER PARA O NOME DO USUÁRIO: system("getent passwd " + hash.value("Uid"))
   listaItem << new QStandardItem(newHash.value("Threads"));
   listaItem << new QStandardItem(newHash.value("ContextSwitches"));
   model->appendRow(listaItem);

  ui->tableView->setModel(model);
  ui->tableView->setShowGrid(false);
  ui->tableView->setAlternatingRowColors(true);
  ui->tableView->verticalHeader()->setVisible(false);
  ui->tableView->setSortingEnabled(true);
  ui->tableView->sortByColumn(4, Qt::AscendingOrder);

}

void MainWindow::setCPUgraph(){
    int nCPU = threadCPU->getNumCPUS();
    cpuData.resize(nCPU);
    for(int i = 0; i < nCPU; i++){
        cpuData[i].resize(60);
        cpuData[i].fill(0);
    }
    ui->cpuGraph->yAxis->setRange(0, 100);
    ui->cpuGraph->yAxis->setAutoTickStep(false);
    ui->cpuGraph->yAxis->setTickStep(50);
    ui->cpuGraph->xAxis->setRange(0, 60);
    ui->cpuGraph->xAxis->setTickStep(15);
}

void MainWindow::updateCPU(QVector<double> percent){  //VETOR DE CPUS?!?!
    QString currentCPU, name = "CPU ";
    double value;
    int i = 0;
    for (int i = 0; i < threadCPU->getNumCPUS(); i++){
        //std::cout <<"value: " << value << std::endl;
        for (int pos = 0; pos < 59; ++pos){
            value = cpuData[i][pos+1];
            cpuData[i].replace(pos, value);
        }
        cpuData[i][59] = percent[i];
        std::cout << cpuData[i][59] << std::endl;
        ui->cpuGraph->addGraph();
        //currentCPU.setNum(i+1);
        //name += currentCPU;
        //ui->cpuGraph->graph(i)->setName(name);

        //ESCOLHENDO A COR
        switch (i){
        case 0:
            ui->cpuGraph->graph(i)->setPen(QPen(Qt::blue));
            break;
        case 1:
            ui->cpuGraph->graph(i)->setPen(QPen(Qt::red));
            break;
        case 2:
            ui->cpuGraph->graph(i)->setPen(QPen(Qt::green));
            break;
        case 3:
            ui->cpuGraph->graph(i)->setPen(QPen(Qt::yellow));
            break;
        case 4:
            ui->cpuGraph->graph(i)->setPen(QPen(Qt::black));
            break;
        case 5:
            ui->cpuGraph->graph(i)->setPen(QPen(Qt::cyan));
            break;
        case 6:
            ui->cpuGraph->graph(i)->setPen(QPen(Qt::magenta));
            break;
        case 7:
            ui->cpuGraph->graph(i)->setPen(QPen(Qt::gray));
            break;
        case 8:
            ui->cpuGraph->graph(i)->setPen(QPen(Qt::darkRed));
            break;
        }
        ui->cpuGraph->graph(i)->setData(x, cpuData[i]);
        ui->cpuGraph->replot();
    }
}

void MainWindow::on_pushButton_clicked()
{
    //listaitem. (....) ?
}
