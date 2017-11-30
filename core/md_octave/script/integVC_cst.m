%   Construction de la matrice M_int_Lin.
%   On integre sur le sous-Tetrahedre de volume v/4 
%   ENTREE :
%   x,y,z   : Tableaux des coordonnees.
%   kne    : Tableau des noeuds associes a chaque element.
%   nbel     : Nombre d'elements Tetra.
%   nn    : Nombre de noeuds.


% *
% *   SORTIE :
% *
% *   Matrice des coefficient de l'integration par interpolation lineaire
% * sur des Tetrahedres.
% *
% *   SOUS-PROGRAMMES :
% *
% *   volumeTetra
% *
% c----$---1---------2---------3---------4---------5---------6---------7-c

function [M_int_cst]= integVC_cst(V_VC)

         ia = [];        ja = [];        s = [];
         nDoFr=size(V_VC,1);
         M_int_cst = sparse ( ia, ja, s, nDoFr,nDoFr,1 );

%xn=zeros(4,1);yn=xn; zn=xn; %ks=xn;
  

%for in=1:nn
%    M_int_cst(in,in)=V_VC(in);
%end
M_int_cst=diag(V_VC);
end
                
%===========================================================

