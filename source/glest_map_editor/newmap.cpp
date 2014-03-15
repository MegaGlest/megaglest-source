#include "newmap.h"
#include "ui_newmap.h"

NewMap::NewMap(Renderer *renderer, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewMap),
    renderer(renderer)
{
    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(create()));//create new map
}

NewMap::~NewMap()
{
    delete ui;
}

void NewMap::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void NewMap::create(){
    cout << "create: " << ui->inputAltitude->text().toStdString () << "; " << ui->inputHeight->text().toStdString () << "; " << ui->inputWidth->text().toStdString () << "; " << ui->inputPlayers->text().toStdString () << "; " << ui->inputSurface->currentIndex() << "; " << endl;
}
