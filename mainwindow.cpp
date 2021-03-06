/**
  * qtiRecovery - mainwindow.cpp
  * Copyright (C) 2011 Krystof Celba
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"
#include "QCompleter"
#include "QFile"
#include "QList"



#ifdef __APPLE__
    #define FILE_HISTORY_PATH "../Resources/history"
#else
    #define FILE_HISTORY_PATH "history"
#endif


MainWindow *thisPointer;

int received_cb(irecv_client_t client, const irecv_event_t* event);
int progress_cb(irecv_client_t client, const irecv_event_t* event);
int precommand_cb(irecv_client_t client, const irecv_event_t* event);
int postcommand_cb(irecv_client_t client, const irecv_event_t* event);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    thisPointer = this;

    this->loadHistory();

    connect(ui->actionDevice_info , SIGNAL(triggered()), this, SLOT(showDeviceInfo()));
    connect(ui->pushButton , SIGNAL(clicked()), this, SLOT(sendCommand()));

    irecv_init();
    client = NULL;
    for (int i = 0; i <= 5; i++)
    {
            printf("Attempting to connect... \n");

            if (irecv_open(&client) != IRECV_E_SUCCESS)
                    sleep(1);
            else
                    break;

            if (i == 5) {
                    exit(-1);
            }
    }

    if(client)
    {
        irecv_event_subscribe(client, IRECV_PROGRESS, &progress_cb, NULL);
        irecv_event_subscribe(client, IRECV_RECEIVED, &received_cb, NULL);
        irecv_event_subscribe(client, IRECV_PRECOMMAND, &precommand_cb, NULL);
        irecv_event_subscribe(client, IRECV_POSTCOMMAND, &postcommand_cb, NULL);

        irecv_get_device(client, &device);

        QString statusString = QString("");
        unsigned char srnm[12], imei[15];

        int ret = irecv_get_srnm(client, srnm);
        if(ret == IRECV_E_SUCCESS)
        {
            statusString.sprintf("%s connected. Serial number: %s", device->product, srnm);
            ui->deviceInfoLabel->setText(statusString);
        }
    }
}


void MainWindow::showDeviceInfo()
{
    QString info = QString("");
    int ret;
    unsigned int cpid, bdid;
    unsigned long long ecid;
    unsigned char srnm[12], imei[15];


    info.sprintf("%sProduct: %s\n", info.toAscii().data(), device->product);
    info.sprintf("%sModel: %s\n", info.toAscii().data(), device->model);

    ret = irecv_get_cpid(client, &cpid);
    if(ret == IRECV_E_SUCCESS)
    {
        info.sprintf("%sCPID: %d\n", info.toAscii().data(), cpid);
    }

    ret = irecv_get_bdid(client, &bdid);
    if(ret == IRECV_E_SUCCESS)
    {
        info.sprintf("%sBDID: %d\n", info.toAscii().data(), bdid);
    }

    ret = irecv_get_ecid(client, &ecid);
    if(ret == IRECV_E_SUCCESS)
    {
        info.sprintf("%sECID: %lld\n", info.toAscii().data(), ecid);
    }

    ret = irecv_get_srnm(client, srnm);
    if(ret == IRECV_E_SUCCESS)
    {
        info.sprintf("%sSRNM: %s\n", info.toAscii().data(), srnm);
    }

    ret = irecv_get_imei(client, imei);
    if(ret == IRECV_E_SUCCESS)
    {
        info.sprintf("%sIMEI: %s\n", info.toAscii().data(), imei);
    }


    QMessageBox *dialog = new QMessageBox(this);
    dialog->setText(info);
    dialog->exec();
}

void MainWindow::loadHistory()
{
    QFile file(FILE_HISTORY_PATH);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QByteArray line = file.readLine();
    QList<QString> list;
    list.append(line);
    while (!line.isNull())
    {
        line = file.readLine();
        if(line.count()!=0)
        {
            line.remove(line.count()-1, 1);
            list.append(line);
        }
    }
    for(int i = list.count()-1; i >= 0; i--)
    {
        ui->commandLine->addItem(list.at(i));
    }
}

void appendToHistory(QString text)
{
    QFile file(FILE_HISTORY_PATH);
    text.append("\n");
    if (file.open(QIODevice::ReadWrite | QIODevice::Append))
    {
        file.write((const char *)text.toAscii().data());

    }
    file.close();
}

void MainWindow::sendCommand()
{
    ui->commandLine->addItem(ui->commandLine->currentText());

    appendToHistory(ui->commandLine->currentText());

    QString tmp = QString("> ");
    tmp.append(ui->commandLine->currentText());
    ui->textEdit->append(tmp);

    char *cmd = ui->commandLine->currentText().toAscii().data();
    int error = irecv_send_command(client, cmd);
    if (error != IRECV_E_SUCCESS)
    {
        qApp->quit();
    }
}



int received_cb(irecv_client_t client, const irecv_event_t* event)
{
    printf("received\n");
    if (event->type == IRECV_RECEIVED) {
            int i = 0;
            int size = event->size;
            char* data = event->data;
            for (i = 0; i < size; i++)
            {
                printf("%c", data[i]);
            }
    }
    return thisPointer->received_cb_g(client, event);
}

int precommand_cb(irecv_client_t client, const irecv_event_t* event)
{
    return thisPointer->precommand_cb_g(client, event);
}

int postcommand_cb(irecv_client_t client, const irecv_event_t* event)
{
    return thisPointer->postcommand_cb_g(client, event);
}

int progress_cb(irecv_client_t client, const irecv_event_t* event)
{
    return thisPointer->progress_cb_g(client, event);
}


int MainWindow::received_cb_g(irecv_client_t client, const irecv_event_t* event)
{
    if (event->type == IRECV_RECEIVED) {
            int i = 0;
            int size = event->size;
            char* data = event->data;
            for (i = 0; i < size; i++)
            {
                QString tmp = QString("");
                printf("%c", data[i]);
                tmp.sprintf("%s%c", tmp.toAscii().data(), data[i]);
                ui->textEdit->append(tmp);
            }
    }
    return 0;
}
int MainWindow::precommand_cb_g(irecv_client_t client, const irecv_event_t* event)
{
    /*if (event->type == IRECV_PRECOMMAND) {
            irecv_error_t error = 0;
            if (event->data[0] == '/') {
                    parse_command(client, event->data, event->size);
                    return -1;
            }
    }*/
    return 0;
}

int MainWindow::postcommand_cb_g(irecv_client_t client, const irecv_event_t* event)
{
    char* value = NULL;
    char* action = NULL;
    char* command = NULL;
    char* argument = NULL;
    irecv_error_t error = IRECV_E_SUCCESS;

    if (event->type == IRECV_POSTCOMMAND) {
            command = strdup(event->data);
            action = strtok(command, " ");
            if (!strcmp(action, "getenv")) {
                    argument = strtok(NULL, " ");
                    error = irecv_getenv(client, argument, &value);
                    if (error != IRECV_E_SUCCESS) {
                            //debug("%s\n", irecv_strerror(error));
                            free(command);
                            return error;
                    }
                    QString tmp = QString("");
                    tmp.sprintf("%s%s\n", tmp.toAscii().data(), value);
                    ui->textEdit->append(tmp);
                    free(value);
            }

            if (!strcmp(action, "reboot")) {
                qApp->quit();
            }
    }

    if (command) free(command);
    return 0;
}

int MainWindow::progress_cb_g(irecv_client_t client, const irecv_event_t* event)
{
    /*if (event->type == IRECV_PROGRESS) {
            print_progress_bar(event->progress);
    }
    */
    return 0;

}


MainWindow::~MainWindow()
{
    irecv_close(client);
    irecv_exit();
    delete ui;
}
