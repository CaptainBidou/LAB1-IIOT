# code pour modifier les csv on passe en argument le nom du fichier
# puis le code remplace les séparateurs , par des ; et modifie les décimales . par des ,

import sys
import os
import csv

def modify_csv(file_path):
    # Vérifier si le fichier existe
    if not os.path.isfile(file_path):
        print(f"Le fichier {file_path} n'existe pas.")
        return

    # Lire le contenu du fichier CSV
    with open(file_path, 'r', newline='', encoding='utf-8') as csvfile:
        reader = csv.reader(csvfile)
        rows = list(reader)

    # Modifier les données
    modified_rows = []
    for row in rows:
        modified_row = []
        for item in row:
            # Remplacer les points par des virgules pour les décimales
            item = item.replace('.', ',')
            modified_row.append(item)
        modified_rows.append(modified_row)

    # Écrire les données modifiées dans un nouveau fichier CSV
    new_file_path = f"modified_{os.path.basename(file_path)}"
    with open(new_file_path, 'w', newline='', encoding='utf-8') as csvfile:
        writer = csv.writer(csvfile, delimiter=';')
        writer.writerows(modified_rows)

    print(f"Le fichier modifié a été enregistré sous {new_file_path}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python CSV.py <chemin_du_fichier_csv>")
    else:
        modify_csv(sys.argv[1])
    
