import os
import xml.etree.ElementTree as ET

def count_functions_with_docs(xml_dir):
    total_functions = 0
    documented_functions = 0

    for dirPath, _, files in os.walk(xml_dir):
        for file in files:
            if file.endswith('.xml'):
                tree = ET.parse(os.path.join(dirPath, file))
                root = tree.getroot()

                # Look for <memberdef> tags with kind="function"
                for member in root.findall(".//memberdef[@kind='function']"):
                    total_functions += 1
                    # Check if the function has documentation
                    if member.find("briefdescription").find("para") is not None or \
                       member.find("detaileddescription").find("para") is not None:
                        documented_functions += 1

    return total_functions, documented_functions

def main():
    xml_dir = "../engine/docs/xml"  # Replace with the path to your Doxygen XML output directory
    total, documented = count_functions_with_docs(xml_dir)
    if total > 0:
        print(xml_dir)
        print(f"Total functions: {total}")
        print(f"Documented functions: {documented}")
        print(f"Percentage documented: {documented / total * 100:.2f}%")
    else:
        print("No functions found in the XML directory.")
    xml_dir = "../web/backend/docs/xml/"  # Replace with the path to your Doxygen XML output directory
    total, documented = count_functions_with_docs(xml_dir)
    if total > 0:
        print(xml_dir)
        print(f"Total functions: {total}")
        print(f"Documented functions: {documented}")
        print(f"Percentage documented: {documented / total * 100:.2f}%")
    else:
        print("No functions found in the XML directory.")
    xml_dir = "../web/java-driver/docs/xml/"  # Replace with the path to your Doxygen XML output directory
    total, documented = count_functions_with_docs(xml_dir)
    if total > 0:
        print(xml_dir)
        print(f"Total functions: {total}")
        print(f"Documented functions: {documented}")
        print(f"Percentage documented: {documented / total * 100:.2f}%")
    else:
        print("No functions found in the XML directory.")

if __name__ == "__main__":
    main()
