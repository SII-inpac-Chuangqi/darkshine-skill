#include <TH1D.h>
#include <TFile.h>
#include <TMath.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <iostream>
using namespace std;

// DarkSHINE Baseline 1.6 — 90% CL limit setting
// Reads fullcutflow histograms from signal fullout files
// Output: sigcutflow_1.6.png, limit_1.6.png
//
// Usage: root -l -q -b runLimit.C

void runLimit(){
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);

    const int nP = 10;
    string CH[10] = {"0001MeV","0010MeV","0020MeV","0050MeV","0100MeV",
                     "0200MeV","0500MeV","1000MeV","1500MeV","2000MeV"};
    // Cross sections (pb/ε²) for 4 GeV e⁻ on W, CalcHEP
    double xs[10] = {9.20E+13,5.82E+11,1.52E+11,1.95E+10,3.16E+09,
                     4.29E+08,1.17E+07,1.19E+05,1.50E+04,2.50E+03};
    double x[10] = {0.001,0.01,0.02,0.05,0.1,0.2,0.5,1.0,1.5,2.0};

    char fn[256];
    double yAll[nP], yDQ[nP], y1trk[nP], yMiss[nP], yHCal[nP], yCell[nP], yECal[nP];
    string path="/cefs/higgs/zhuyifan/DarkSHINE/input/signal/";

    // fullcutflow bins: 1=All 2=passDQ 3=1track 4=MissingP 5=HCal 6=HCalCell10 7=HCalCell0.1 8=ECal
    for(int i=0;i<nP;i++){
        sprintf(fn,"%sana_signal_%s_fullout.root",path.c_str(),CH[i].c_str());
        TFile *f=new TFile(fn);
        TH1D *h=(TH1D*)f->Get("fullcutflow");
        double all=h->GetBinContent(1);
        yAll[i]  = 1.;
        yDQ[i]   = h->GetBinContent(2)/all;
        y1trk[i] = h->GetBinContent(3)/all;
        yMiss[i] = h->GetBinContent(4)/all;
        yHCal[i] = h->GetBinContent(5)/all;
        yCell[i] = h->GetBinContent(7)/all;
        yECal[i] = h->GetBinContent(8)/all;
        cout<<CH[i]<<" all="<<(int)all<<" sigEff="<<yECal[i]<<endl;
        f->Close();
    }

    // Cutflow plot
    TCanvas *c1=new TCanvas("c1","",1000,600);
    c1->SetLogx();
    TGraph *g0=new TGraph(nP,x,yAll);
    TGraph *g1=new TGraph(nP,x,yDQ);
    TGraph *g2=new TGraph(nP,x,y1trk);
    TGraph *g3=new TGraph(nP,x,yMiss);
    TGraph *g4=new TGraph(nP,x,yHCal);
    TGraph *g5=new TGraph(nP,x,yCell);
    TGraph *g6=new TGraph(nP,x,yECal);

    int colors[]={1,kSpring+4,kOrange+7,kAzure+7,kViolet+6,kPink+8,kRed+2};
    TGraph* gs[]={g0,g1,g2,g3,g4,g5,g6};
    string labels[]={"All","passDQ","1 tag+1 rec","p_{miss}>4GeV","HCal<30MeV","HCalCell<0.1MeV","ECal<2.5GeV"};
    for(int i=0;i<7;i++){
        gs[i]->SetLineColor(colors[i]); gs[i]->SetMarkerColor(colors[i]);
        gs[i]->SetMarkerStyle(20+i); gs[i]->SetMarkerSize(1.0);
    }
    g0->GetXaxis()->SetTitle("m_{A'} [GeV]");
    g0->GetYaxis()->SetTitle("Cumulative efficiency");
    g0->GetHistogram()->SetMaximum(1.1); g0->GetHistogram()->SetMinimum(0.);
    g0->Draw("APL");
    for(int i=1;i<7;i++) gs[i]->Draw("PL same");

    TLegend *leg=new TLegend(0.12,0.12,0.88,0.32);
    leg->SetTextSize(0.025); leg->SetBorderSize(0); leg->SetNColumns(2); leg->SetMargin(0.05);
    for(int i=0;i<7;i++) leg->AddEntry(gs[i],labels[i].c_str(),"lp");
    leg->Draw();
    c1->Print("/cefs/higgs/zhuyifan/DarkSHINE/ds-analysis/plots/Baseline1p6/sigcutflow_1.6.png");

    // Limits
    double EOT=3e14, Nb1year=0.528, Nb=Nb1year;
    double Ups=0.5*TMath::ChisquareQuantile(0.95,2*(Nb+1))-Nb;
    cout<<"\n=== 90% CL (EOT="<<EOT<<", Nb="<<Nb<<", Ups="<<Ups<<") ==="<<endl;
    cout<<"mA'[GeV]  sigEff  Nlim      epsilon^2"<<endl;
    double Ep2[10];
    for(int i=0;i<nP;i++){
        double Nexp=Ups/yECal[i];
        Ep2[i]=xs[i]?Nexp/(xs[i]*0.1*6.76*EOT*6.02E23/184*1E-36):0;
        printf("  %.3f    %.3f   %.2e  %.2e\n",x[i],yECal[i],Nexp,Ep2[i]);
    }

    TCanvas *c2=new TCanvas("c2","",800,600);
    c2->SetLogx(); c2->SetLogy();
    TGraph *gL=new TGraph(nP,x,Ep2);
    gL->SetMarkerStyle(20); gL->SetMarkerSize(1.2);
    gL->SetLineColor(kRed+1); gL->SetMarkerColor(kRed+1);
    gL->GetXaxis()->SetTitle("m_{A'} [GeV]");
    gL->GetYaxis()->SetTitle("#varepsilon^{2}");
    gL->GetHistogram()->SetMaximum(1e-3); gL->GetHistogram()->SetMinimum(1e-12);
    gL->Draw("APL");
    c2->Print("/cefs/higgs/zhuyifan/DarkSHINE/ds-analysis/plots/Baseline1p6/limit_1.6.png");
    cout<<"=== Done ==="<<endl;
}
