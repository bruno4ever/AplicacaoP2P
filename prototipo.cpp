#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include <regex>
#include <unordered_set>

std::vector<std::string> hashesregras;
std::unordered_set<std::string> hashesvotados;
std::unordered_set<std::string> hashesdescartados;
std::unordered_set<std::string> substituicoes;
std::unordered_set<std::string> hasheslidas;

// tags iniciais e suas subtags 
std::map<std::string, std::vector<std::string>> tags = {
        {"Barulho", {"Horário", "Local", "Datas"}},
        {"Animais", {"Animais permitidos", "Quantidade", "Locais permitidos"}},
        {"Limpeza", {"Local de Descarte", "Horário de Coleta"}},
        {"Segurança", {"Horário de circulação", "Horário de visitas"}}
    };
    
    
// funçao pra chamar o shell
std::string comando(const std::string& comando) {
    std::string resultado;
    char buffer[128];
    FILE* pipe = popen(comando.c_str(), "r");

    if (!pipe) return "ERRO";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        resultado += buffer;
    }
    pclose(pipe);
    return resultado;
}


// limpar posts pra nao dar conflito
std::string limpar(const std::string& entrada) {
    std::string limpa = entrada;
    limpa = std::regex_replace(limpa, std::regex("SUBTAG"), "");
    limpa = std::regex_replace(limpa, std::regex("TAG"), "");
    limpa = std::regex_replace(limpa, std::regex("[:|]"), "");
    limpa = std::regex_replace(limpa, std::regex("PROPOSTA DE NOVA REGRA"), "");
    limpa = std::regex_replace(limpa, std::regex("PROPOSTA DE ALTERAÇÃO"), "");
    limpa = std::regex_replace(limpa, std::regex("'"), "");
    limpa = std::regex_replace(limpa, std::regex("PROPOSTA DE TAG"), "");
    limpa = std::regex_replace(limpa, std::regex("PROPOSTA DE SUBTAG"), "");
    return limpa;
}


// pegar tempo do post
long long timestampbloco(const std::string& hash) {
    std::string timestamp = comando("./freechains chain '#regras' get block " + hash + " | grep '\"time\"' | cut -d ':' -f2 | tr -d ' ,'");
    return std::stoll(timestamp);
}



// pegar tempo atual
long long timestampagora() {
    std::string timeagora = comando("./freechains-host now");
    return std::stoll(timeagora);
}


// checar reputaçao do post
int repRegra(const std::string& hash) {
    std::string reputacao = comando("./freechains chain '#regras' reps " + hash);
    return std::stoi(reputacao);
}


// atualizar as regras em vigor

int regrasVigor() {
    
    std::string hashes1 = comando("./freechains chain '#regras' consensus");
    std::istringstream iss(hashes1);
    std::vector<std::string> hashes;
    std::string hash;
    std::vector<std::string> hashesposts;
    std::vector<std::string> hashesAlt;
    std::unordered_set<std::string> propostasalt;
    std::map<std::string, std::string> melhorcomtag;
    hashesregras.clear();
    
    while (iss >> hash) {
        hashes.push_back(hash);
    }

    if (hashes.empty()) {
        
        system("clear");
        return 0;
    }
    
    
        
    for (int i = 0; i < hashes.size(); ++i) {
        
        // hash de blocos descartados ou tags lidas (hasheslidas) da chain sao ignorados
        // hash de "patches" aprovados sao automaticamente adicionados (substituicoes)
        
        if (hashesdescartados.count(hashes[i])) {
            continue;
        }
        
        if (substituicoes.count(hashes[i])) {
            hashesposts.push_back(hashes[i]);
            continue;
        }
        
        if (hasheslidas.count(hashes[i])) {
            continue;
        }
        
        // pega todas as propostas de nova regra
        
        std::string conteudo = comando("./freechains chain '#regras' get payload " + hashes[i] + " | grep '\\[PROPOSTA DE NOVA REGRA\\]'");
        if (conteudo.empty()) {
            continue;
        }
        
        // comparar as tags das propostas de nova regra evitando conflitos
        // caso tenha duas propostas com as mesmas tags, decidi que a com maior reputaçao fica na rede
        
        size_t tagPos = conteudo.find("TAG: ");
        size_t subtagPos = conteudo.find("SUBTAG: ");
        std::string tag = conteudo.substr(tagPos + 5, conteudo.find('|', tagPos) - (tagPos + 5));
        std::string subtag = conteudo.substr(subtagPos + 8, conteudo.find('|', subtagPos) - (subtagPos + 8));
        std::string chave = tag +  subtag;
        int reputacaoAtual = repRegra(hashes[i]);
        
        if (!melhorcomtag.count(chave)) {
        melhorcomtag[chave] = hashes[i];
            
        } 
        else {
            std::string hashAgora = melhorcomtag[chave];
            int reputacaoMelhor = repRegra(hashAgora);
            if (reputacaoAtual > reputacaoMelhor) {
                hashesdescartados.insert(hashAgora);
                hashesvotados.insert(hashAgora);
                melhorcomtag[chave] = hashes[i];
            }
            else {
                hashesdescartados.insert(hashes[i]);
                hashesvotados.insert(hashes[i]);
           }
        }
    }
    
    for (const auto& par : melhorcomtag) {
    hashesposts.push_back(par.second);
    }
    
    // se nao existirem posts, retorna 0
    
    if (hashesposts.empty()) 
    {
        system("clear");
        return 0;
    }
    
    
    // se a reputaçao da proposta for positiva e o post for de mais de 24 horas atrás, a regra entra em vigor
    
    for (int i = 0; i < hashesposts.size(); ++i) {
            if ((timestampagora() - timestampbloco(hashesposts[i]) > 86400000) && (repRegra(hashesposts[i]) > 0)) {
                hashesvotados.insert(hashesposts[i]);
                hashesregras.push_back(hashesposts[i]);
            }
        }
        
    // pego somente as propostas de alteraçao, coloquei em outro for pois precisei coletar as propostas originais primeiro
    
    for (int i = 0; i < hashes.size(); ++i) {
        
        if (hashesdescartados.count(hashes[i])) continue;
        if (hasheslidas.count(hashes[i])) continue;
        if (hashesvotados.count(hashes[i])) continue;
        
        std::string conteudo = comando("./freechains chain '#regras' get payload " + hashes[i] + " | grep '\\[PROPOSTA DE ALTERAÇÃO\\]'");

        if (!conteudo.empty()) {
            hashesAlt.push_back(hashes[i]);
        }
    }
    

        
    for (int i = 0; i < hashesregras.size(); ++i) { 
        std::string melhorHash = hashesregras[i];
        int maxLikes = repRegra(hashesregras[i]); 
        for (int j = 0; j < hashesAlt.size(); ++j) { 
            
            // considero todas as propostas de alteraçao para a mesma regra
            std::string conteudo = comando("./freechains chain '#regras' get payload " + hashesAlt[j]);
            if (conteudo.find("REFERÊNCIA: " + hashesregras[i]) != std::string::npos) {
                int likes = repRegra(hashesAlt[j]); 
                if ((likes > maxLikes) && (timestampagora() - timestampbloco(hashesAlt[j]) > 86400000)) { 
                    maxLikes = likes;
                    melhorHash = hashesAlt[j];
                }
            }
        }
        
        // se sua reputaçao for maior que a regra em vigor e o tempo do post for a mais de 24 horas, substitui a regra original
        // descarto as outras propostas de alteraçao com menor reputaçao e a regra original
        
        if (melhorHash != hashesregras[i]) {
            hashesvotados.insert(melhorHash);
            hashesdescartados.insert(hashesregras[i]);
            hashesregras[i] = melhorHash;
            substituicoes.insert(melhorHash);
        }
    }
    if (hashesregras.empty()) {
        
        // se nenhuma regra está em vigor, retorno 0
        
        return 0;
    }
    return 1;
}

// atualizar tags e subtags do sistema

void TagsemVigor() {
    
    
    std::string hashes1 = comando("./freechains chain '#regras' consensus");
    std::istringstream iss(hashes1);
    std::vector<std::string> hashes;
    std::string hash;
    std::vector<std::string> hashestags;
    std::vector<std::string> hashessubtags;
    std::string novatag;
    std::string tagref;
    std::string novasubtag;
    
    while (iss >> hash) {
        hashes.push_back(hash);
    }

    if (hashes.empty()) {
        return;
    }
        
    for (int i = 0; i < hashes.size(); ++i) {
        
        // descartando tags já lidas, hashes descartados e hashes de patch de proposta (só quero propostas de tag aqui)
        
        if (hashesdescartados.count(hashes[i])) {
            continue;
        }
        
        if (substituicoes.count(hashes[i])) {
            continue;
        }
        
        if (hasheslidas.count(hashes[i])) {
            continue;
        }
        
        // seleciono todas as propostas de tag e subtag da cadeia
        
        std::string conteudo = comando("./freechains chain '#regras' get payload " + hashes[i] + " | grep '\\[PROPOSTA DE TAG\\]'");
        if (!conteudo.empty()) {
            hashestags.push_back(hashes[i]);
        }
        
        conteudo = comando("./freechains chain '#regras' get payload " + hashes[i] + " | grep '\\[PROPOSTA DE SUBTAG\\]'");
        if (!conteudo.empty()) {
            hashessubtags.push_back(hashes[i]);
        }
    }
    
    
    // percorro todas as proposta de tag, caso sua avaliaçao seja positiva e postada a mais de 24 horas, a tag é aceita na rede 
    
    for (int i = 0; i < hashestags.size(); ++i) {
        
        if ((timestampagora() - timestampbloco(hashestags[i]) > 86400000) && (repRegra(hashestags[i]) > 0)) {
            novatag = comando("./freechains chain '#regras' get payload " + hashestags[i] + " | sed -E \"s/^\\[PROPOSTA DE TAG\\]: *//\" | sed 's/[[:space:]]*$//'"); 
            novatag.erase(std::remove_if(novatag.begin(), novatag.end(), ::isspace), novatag.end());
            tags[novatag] = {};
            hashesvotados.insert(hashestags[i]);
            hasheslidas.insert(hashestags[i]);
        }
    }
    
    // percorro todas as proposta de subtag, caso sua avaliaçao seja positiva e postada a mais de 24 horas, a subtag é aceita na rede 
    
    for (int i = 0; i < hashessubtags.size(); ++i) {
        
        if ((timestampagora() - timestampbloco(hashessubtags[i]) > 86400000) && (repRegra(hashessubtags[i]) > 0)) {
            tagref = comando("./freechains chain '#regras' get payload " + hashessubtags[i] + " | sed -E 's/^\\[PROPOSTA DE SUBTAG\\]: TAG: *([^ ]+) .*/\\1/'"); 
            tagref.erase(std::remove_if(tagref.begin(), tagref.end(), ::isspace), tagref.end());
            novasubtag = comando("./freechains chain '#regras' get payload " + hashessubtags[i] + " | sed -nE 's/^\\[PROPOSTA DE SUBTAG\\]: TAG: .* SUBTAG: *(.*)/\\1/p'");
            novasubtag.erase(std::remove_if(novasubtag.begin(), novasubtag.end(), ::isspace), novasubtag.end());
            tags[tagref].push_back(novasubtag);
            hashesvotados.insert(hashessubtags[i]);
            hasheslidas.insert(hashessubtags[i]);
        }
    }
    
}


// funçao de publicar propostas

void proporRegra(const std::string& pub, const std::string& pvt) {
    
    system("clear");
    std::cout << "Carregando...\n";
    TagsemVigor();
    system("clear");
    
    std::string tag; 
    std::string subtag; 
    std::string proposta; 
    std::string punicao;
    std::string tag1;
    std::string subtag1;
    
    std::cout << "Tags e Subtags válidas:\n";
    
    for (const auto& [tag, subtags] : tags) {
        std::cout << " - " << tag << ": ";
        for (size_t i = 0; i < subtags.size(); ++i) {
            std::cout << subtags[i];
            if (i != subtags.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
    }
    
    std::cout << "\n\nInforme a TAG da regra: \n ";
    std::getline(std::cin, tag);

    if (tags.find(tag) == tags.end()) {
        system("clear");
        std::cout << "ERRO: Insira uma TAG válida. Sua proposta foi recusada.\n";
        return;
    }
    
    std::cout << "Informe a SUBTAG da regra: \n ";
    std::getline(std::cin, subtag);

    const std::vector<std::string>& subtagsP = tags[tag];
    if (std::find(subtagsP.begin(), subtagsP.end(), subtag) == subtagsP.end()) {
        system("clear");
        std::cout << "ERRO: Insira uma SUBTAG válida para a TAG inserida. Sua proposta foi recusada.\n";
        return;
    }
    
    std::string tipoP = "[PROPOSTA DE NOVA REGRA]:  ";
    std::cout << "Especifique sua regra: \n> ";
    std::getline(std::cin, proposta);
    proposta = limpar(proposta);
    std::cout << "Escreva uma puniçao para quem quebrar essa regra: \n> ";
    std::getline(std::cin, punicao);
    punicao = limpar(punicao);
    regrasVigor();
    std::string hashExiste;
    
    // checo se ja existe uma regra em vigor com as mesmas tags
    
    for (int i = 0; i < hashesregras.size(); ++i) {
        std::string conteudo = comando("./freechains chain '#regras' get payload " + hashesregras[i]);
        std::smatch match;
        std::regex tagregex("TAG: ([^|]+) \\| SUBTAG: ([^|]+)");

        if (std::regex_search(conteudo, match, tagregex) && match.size() >= 3) {
            tag1 = std::regex_replace(match[1].str(), std::regex("^ +| +$"), "");
            subtag1 = std::regex_replace(match[2].str(), std::regex("^ +| +$"), "");
        }
        if (tag1 == tag && subtag1 == subtag) {
            hashExiste = hashesregras[i];
            break;
        }
    }

    std::string conteudo;
    
    // caso exista, a proposta é publicada como proposta de alteração
    // caso nao exista, a proposta é publicada normalmente
    
    if (!hashExiste.empty()) {
        std::cout << "\nJá existe uma regra em vigor com essa Tag e Subtag. Sua proposta será considerada uma proposta de alteração\n";
        conteudo = "[PROPOSTA DE ALTERAÇÃO]: REFERÊNCIA: " + hashExiste +
                   " | TAG: " + tag + " | SUBTAG: " + subtag +
                   " | REGRA: " + proposta + " | PUNIÇÃO: " + punicao;
    } else {
        conteudo = "[PROPOSTA DE NOVA REGRA]: TAG: " + tag + " | SUBTAG: " + subtag +
                   " | REGRA: " + proposta + " | PUNIÇÃO: " + punicao;
    }
    
    comando("./freechains chain '#regras' post --sign=" + pvt + " inline '" + conteudo + "'" + " > /dev/null 2>&1");
    std::cout << "Proposta publicada com sucesso\n\n";
    return;
}

// funçao de propor alteraçoes

void proporAlterar(const std::string& pub, const std::string& pvt) {
    
    system("clear");
    std::cout << "Carregando...\n\n";

    // se a funçao de regras em vigor retornar 0, nao existe regras para proporAlterar
    
    if (regrasVigor() == 0) {
        std::cout << "\n\nNenhuma regra está em vigor\n\n";
        return;
    }
    
    system("clear");
    
    // exibindo regras em Vigor
    
    std::cout << " ------------- Regras em Vigor ------------- \n\n";

    // formatando as regras para exibiçao
    
    for (int i = 0; i < hashesregras.size(); ++i) {
            std::string payload = "./freechains chain '#regras' get payload " + hashesregras[i] +
    " | sed -E \"s/^\\[PROPOSTA DE NOVA REGRA\\]: *//; s/^\\[PROPOSTA DE ALTERAÇÃO\\]: REFERÊNCIA: [^|]* \\| *//\"";
            std::string conteudo = comando(payload);
            std::cout << i + 1 << " - " << "HASH: " << hashesregras[i] << "\n    " << conteudo << "\n\n";
        }

    std::cout << "\nDigite a regra que deseja solicitar alteração (Digite o número da regra ou 0 para cancelar): ";
    int escolha;
    std::string entrada;
    std::getline(std::cin, entrada);
    try {
        escolha = std::stoi(entrada);
        } 
    catch (...) {
            escolha = 999;
        }

    if (escolha == 0) return;
    if (escolha < 1 || escolha > (int)hashesregras.size()) {
        std::cout << "ERRO: Escolha inválida. Retornando ao menu.\n";
        return;
    }
    
    
    // pegando Tag e Subtag da regra em vigor
    
    std::string referencia = hashesregras[escolha - 1];
    std::string original = comando("./freechains chain '#regras' get payload " + referencia);

    std::string tag, subtag;
    std::smatch match;
    std::regex tag_regex("TAG: ([^|]+) \\| SUBTAG: ([^|]+)");

    if (std::regex_search(original, match, tag_regex) && match.size() >= 3) {
        tag = std::regex_replace(match[1].str(), std::regex("^ +| +$"), "");
        subtag = std::regex_replace(match[2].str(), std::regex("^ +| +$"), "");
    }

    std::string especificacao;
    std::string punicao;

    std::cout << "Especifique a nova versão da regra: ";
    std::getline(std::cin, especificacao);
    especificacao = limpar(especificacao);

    std::cout << "Escreva a punição para quem descumprir: ";
    std::getline(std::cin, punicao);
    punicao = limpar(punicao);

    std::string conteudo = "[PROPOSTA DE ALTERAÇÃO]: REFERÊNCIA: " + referencia +
                           " | TAG: " + tag + " | SUBTAG: " + subtag +
                           " | REGRA: " + especificacao + " | PUNIÇÃO: " + punicao;

    comando("./freechains chain '#regras' post --sign=" + pvt + " inline '" + conteudo + "' > /dev/null 2>&1");
    std::cout << "\nProposta de alteração publicada com sucesso\n\n";
    return;
}


// funçao para exibir regras em vigor
void verRegras(const std::string& pub, const std::string& pvt) {
    
    system("clear");
    
    std::cout << "Carregando...\n\n";
    
    if (regrasVigor() == 0) {
        std::cout << "\n\nNenhuma regra está em vigor\n\n";
        return;
    }
    
    system("clear");
    
    // exibindo regras em Vigor
    
    std::cout << " ------------- Regras em Vigor ------------- \n\n";
    
    for (int i = 0; i < hashesregras.size(); ++i) {
            std::string payload = "./freechains chain '#regras' get payload " + hashesregras[i] +
    " | sed -E \"s/^\\[PROPOSTA DE NOVA REGRA\\]: *//; s/^\\[PROPOSTA DE ALTERAÇÃO\\]: REFERÊNCIA: [^|]* \\| *//\"";
            std::string conteudo = comando(payload);
            std::cout << i + 1 << " - " << "HASH: " << hashesregras[i] << "\n    " << conteudo << "\n\n";
        }
        
        std::cout << "\nDeseja avaliar uma regra? (Digite o número da regra ou 0 para pular): ";
        int escolha;
        int opcao1;
        std::string entrada;
        std::getline(std::cin, entrada);
        try { 
            escolha = std::stoi(entrada);
        } 
        catch (...) {
            escolha = 999;
        }

        if (escolha > 0 && escolha - 1 < (int)hashesregras.size()) {
            std::cout << "[1] - Dar like\n";
            std::cout << "[2] - Dar dislike\n";
            std::cout << "[3] - Cancelar\n";
            std::cout << "Digite uma opção: ";
            std::string entrada;
            std::getline(std::cin, entrada);
            try {
                opcao1 = std::stoi(entrada);
                } 
            catch (...) {
                    opcao1 = 999;
                }
        switch (opcao1) {
            case 1: {
                std::string like = comando("./freechains chain '#regras' like " + hashesregras[escolha - 1] + " --sign=" + pvt);
                if (like.find("like must not target itself") != std::string::npos) {
                    std::cout << "Você não pode dar like na sua própria regra.\n\n";
                }
                else if (!like.empty()) {
                    std::cout << "Regra avaliada com sucesso\n\n";
                    
                }
                else {
                    std::cout << "Erro ao avaliar regra\n\n";
                    
                }
                break;
            }
            case 2: {
                std::string dislike = comando("./freechains chain '#regras' dislike " + hashesregras[escolha - 1] + " --sign=" + pvt + " > /dev/null 2>&1");
                if (dislike != "!") {
                    std::cout << "Regra avaliada com sucesso\n\n";
                    
                } else {
                    std::cout << "Erro ao avaliar regra\n\n";
                }
                break;
            }
            case 3: {
                std::cout << "Cancelando...\n\n";
                break;
            }
            default:
                std::cout << "Opção inválida\n";
                break;
            }
        }
        else {
            if (escolha == 0) {
                return;
            }
            std::cout << "ERRO: Escolha inválida. Retornando ao Menu Principal.\n";
            return;
        }
}

// ver reputaçao do usuario
void verRep(const std::string& pub) {
    
    std::string reputacao = comando("./freechains chain '#regras' reps " + pub);
    int reputacao1 = std::stoi(reputacao);
    system("clear");
    std::cout << "Sua reputação: " << reputacao1 << "\n\n";
}

// ver todas as propostas da rede (consensus e heads blocked)

void verPropostas(const std::string& pub, const std::string& pvt) {
    
    system("clear");
    std::cout << "Carregando...\n\n";
    regrasVigor();
    TagsemVigor();
    system("clear");

    std::string hashes1 = comando("./freechains chain '#regras' consensus");
    std::string hashesblocked = comando("./freechains chain '#regras' heads blocked");
    std::string hashes2 = hashes1 + " " + hashesblocked;
    std::istringstream iss(hashes2);
    std::vector<std::string> hashes;
    std::string hash;
    std::vector<std::string> hashesposts;

    while (iss >> hash) {
        hashes.push_back(hash);
    }
    

    if (hashes.empty()) {
        std::cout << "Nenhuma proposta encontrada \n\n";
        return;
    } else {
        
        for (int i = 0; i < hashes.size(); ++i) {
            if (hashesvotados.count(hashes[i])) { 
                    continue;
                }
            std::string conteudo = comando("./freechains chain '#regras' get payload " + hashes[i]);

            if (!conteudo.empty()) {
                hashesposts.push_back(hashes[i]);
            }
        }
        
        if (hashesposts.empty()) {
        std::cout << "Nenhuma proposta encontrada \n\n";
        return;
        } else {
            
            system("clear");
            
            std::cout << "\n\n ------------- Propostas publicadas ------------- \n\n";
            
            for (int i = 0; i < hashesposts.size(); ++i) {
                std::string conteudo = comando("./freechains chain '#regras' get payload " + hashesposts[i]);
                std::cout << i + 1 << " - " << conteudo << "\n";
            }
        }

        std::cout << "\nDeseja avaliar uma proposta específica? (Digite o número da proposta ou 0 para pular): ";
        int escolha;
        int opcao1;
        std::string entrada;
        std::getline(std::cin, entrada);
        try {
            escolha = std::stoi(entrada);
        } 
        catch (...) {
            escolha = 999;
        }

        if (escolha > 0 && escolha - 1 < (int)hashesposts.size()) {
            std::cout << "[1] - Dar like\n";
            std::cout << "[2] - Dar dislike\n";
            std::cout << "[3] - Cancelar\n";
            std::cout << "Digite uma opção: ";
            std::string entrada;
            std::getline(std::cin, entrada);
            try {
                opcao1 = std::stoi(entrada);
                } 
            catch (...) {
                    opcao1 = 999;
                }
        switch (opcao1) {
            case 1: {
                std::string like = comando("./freechains chain '#regras' like " + hashesposts[escolha - 1] + " --sign=" + pvt);
                if (like.find("like must not target itself") != std::string::npos) {
                    std::cout << "Você não pode dar like na sua própria proposta.\n\n";
                }
                else if (like != "!") {
                    std::cout << "Proposta avaliada com sucesso\n\n";
                    
                }
                else {
                    std::cout << "Erro ao avaliar proposta\n\n";
                    
                }
                break;
            }
            case 2: {
                std::string dislike = comando("./freechains chain '#regras' dislike " + hashesposts[escolha - 1] + " --sign=" + pvt + " > /dev/null 2>&1");
                if (dislike != "!") {
                    std::cout << "Proposta avaliada com sucesso\n\n";
                    
                } else {
                    std::cout << "Erro ao avaliar proposta\n\n";
                }
                break;
            }
            case 3: {
                std::cout << "Cancelando...\n\n";
                break;
            }
            default:
                std::cout << "Opção inválida\n";
                break;
            }
        }
        else {
            if (escolha == 0) {
                return;
            }
            std::cout << "ERRO: Escolha inválida. Retornando ao Menu Principal.\n";
            return;
        }
    }
}

// vendo apenas as propostas do consensus

void verPropostasCons(const std::string& pub, const std::string& pvt) {
    
    system("clear");
    std::cout << "Carregando...\n\n";
    regrasVigor();
    TagsemVigor();
    system("clear");

    std::string hashes2 = comando("./freechains chain '#regras' consensus");
    std::istringstream iss(hashes2);
    std::vector<std::string> hashes;
    std::string hash;
    std::vector<std::string> hashesposts;

    while (iss >> hash) {
        hashes.push_back(hash);
    }
    

    if (hashes.empty()) {
        std::cout << "Nenhuma proposta encontrada \n\n";
        return;
    } else {
        
        for (int i = 0; i < hashes.size(); ++i) {
            if (hashesvotados.count(hashes[i])) { 
                    continue;
                }
            std::string conteudo = comando("./freechains chain '#regras' get payload " + hashes[i]);

            if (!conteudo.empty()) {
                hashesposts.push_back(hashes[i]);
            }
        }
        
        if (hashesposts.empty()) {
        std::cout << "Nenhuma proposta encontrada \n\n";
        return;
        } else {
            
            system("clear");
            
            std::cout << "\n\n ------------- Propostas publicadas ------------- \n\n";
            
            for (int i = 0; i < hashesposts.size(); ++i) {
                std::string conteudo = comando("./freechains chain '#regras' get payload " + hashesposts[i]);
                std::cout << i + 1 << " - " << conteudo << "\n";
            }
        }

        std::cout << "\nDeseja avaliar uma proposta específica? (Digite o número da proposta ou 0 para pular): ";
        int escolha;
        int opcao1;
        std::string entrada;
        std::getline(std::cin, entrada);
        try {
            escolha = std::stoi(entrada);
        } 
        catch (...) {
            escolha = 999;
        }

        if (escolha > 0 && escolha - 1 < (int)hashesposts.size()) {
            std::cout << "[1] - Dar like\n";
            std::cout << "[2] - Dar dislike\n";
            std::cout << "[3] - Cancelar\n";
            std::cout << "Digite uma opção: ";
            std::string entrada;
            std::getline(std::cin, entrada);
            try {
                opcao1 = std::stoi(entrada);
                } 
            catch (...) {
                    opcao1 = 999;
                }
        switch (opcao1) {
            case 1: {
                std::string like = comando("./freechains chain '#regras' like " + hashesposts[escolha - 1] + " --sign=" + pvt);
                if (like.find("like must not target itself") != std::string::npos) {
                    std::cout << "Você não pode dar like na sua própria proposta.\n\n";
                }
                else if (like != "!") {
                    std::cout << "Proposta avaliada com sucesso\n\n";
                    
                }
                else {
                    std::cout << "Erro ao avaliar proposta\n\n";
                    
                }
                break;
            }
            case 2: {
                std::string dislike = comando("./freechains chain '#regras' dislike " + hashesposts[escolha - 1] + " --sign=" + pvt + " > /dev/null 2>&1");
                if (dislike != "!") {
                    std::cout << "Proposta avaliada com sucesso\n\n";
                    
                } else {
                    std::cout << "Erro ao avaliar proposta\n\n";
                }
                break;
            }
            case 3: {
                std::cout << "Cancelando...\n\n";
                break;
            }
            default:
                std::cout << "Opção inválida\n";
                break;
            }
        }
        else {
            if (escolha == 0) {
                return;
            }
            std::cout << "ERRO: Escolha inválida. Retornando ao Menu Principal.\n";
            return;
        }
    }
}

// mesma coisa da funçao de ver todas as propostas, mas considerando apenas as bloqueadas

void verPropostasBlocked(const std::string& pub, const std::string& pvt) {
    
    system("clear");
    std::cout << "Carregando...\n\n";
    regrasVigor();
    TagsemVigor();
    system("clear");

    std::string hashes2 = comando("./freechains chain '#regras' heads blocked");
    std::istringstream iss(hashes2);
    std::vector<std::string> hashes;
    std::string hash;
    std::vector<std::string> hashesposts;

    while (iss >> hash) {
        hashes.push_back(hash);
    }
    

    if (hashes.empty()) {
        std::cout << "Nenhuma proposta encontrada \n\n";
        return;
    } else {
        
        for (int i = 0; i < hashes.size(); ++i) {
            if (hashesvotados.count(hashes[i])) { 
                    continue;
                }
            std::string conteudo = comando("./freechains chain '#regras' get payload " + hashes[i]);

            if (!conteudo.empty()) {
                hashesposts.push_back(hashes[i]);
            }
        }
        
        if (hashesposts.empty()) {
        std::cout << "Nenhuma proposta encontrada \n\n";
        return;
        } else {
            
            system("clear");
            
            std::cout << "\n\n ------------- Propostas publicadas ------------- \n\n";
            
            for (int i = 0; i < hashesposts.size(); ++i) {
                std::string conteudo = comando("./freechains chain '#regras' get payload " + hashesposts[i]);
                std::cout << i + 1 << " - " << conteudo << "\n";
            }
        }

        std::cout << "\nDeseja avaliar uma proposta específica? (Digite o número da proposta ou 0 para pular): ";
        int escolha;
        int opcao1;
        std::string entrada;
        std::getline(std::cin, entrada);
        try {
            escolha = std::stoi(entrada);
        } 
        catch (...) {
            escolha = 999;
        }

        if (escolha > 0 && escolha - 1 < (int)hashesposts.size()) {
            std::cout << "[1] - Dar like\n";
            std::cout << "[2] - Dar dislike\n";
            std::cout << "[3] - Cancelar\n";
            std::cout << "Digite uma opção: ";
            std::string entrada;
            std::getline(std::cin, entrada);
            try {
                opcao1 = std::stoi(entrada);
                } 
            catch (...) {
                    opcao1 = 999;
                }
        switch (opcao1) {
            case 1: {
                std::string like = comando("./freechains chain '#regras' like " + hashesposts[escolha - 1] + " --sign=" + pvt);
                if (like.find("like must not target itself") != std::string::npos) {
                    std::cout << "Você não pode dar like na sua própria proposta.\n\n";
                }
                else if (like != "!") {
                    std::cout << "Proposta avaliada com sucesso\n\n";
                    
                }
                else {
                    std::cout << "Erro ao avaliar proposta\n\n";
                    
                }
                break;
            }
            case 2: {
                std::string dislike = comando("./freechains chain '#regras' dislike " + hashesposts[escolha - 1] + " --sign=" + pvt + " > /dev/null 2>&1");
                if (dislike != "!") {
                    std::cout << "Proposta avaliada com sucesso\n\n";
                    
                } else {
                    std::cout << "Erro ao avaliar proposta\n\n";
                }
                break;
            }
            case 3: {
                std::cout << "Cancelando...\n\n";
                break;
            }
            default:
                std::cout << "Opção inválida\n";
                break;
            }
        }
        else {
            if (escolha == 0) {
                return;
            }
            std::cout << "ERRO: Escolha inválida. Retornando ao Menu Principal.\n";
            return;
        }
    }
}

// funçao para publicar propostas de Tag

void proporTag(const std::string& pub, const std::string& pvt) {
    
    system("clear");
    std::cout << "Carregando...\n";
    TagsemVigor();
    system("clear");
    
    std::cout << "Tags e Subtags existentes:\n";
    
    for (const auto& [tag, subtags] : tags) {
        std::cout << " - " << tag << ": ";
        for (size_t i = 0; i < subtags.size(); ++i) {
            std::cout << subtags[i];
            if (i != subtags.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
    }
    
    std::string tag;
    std::cout << "\n\nInforme a nova TAG desejada: \n ";
    std::getline(std::cin, tag);

    if ((tags.find(tag) != tags.end()) || tag.empty() || std::all_of(tag.begin(), tag.end(), isspace)) {
        system("clear");
        std::cout << "ERRO: TAG inválida. Sua proposta foi recusada.\n";
        return;
    }
    
    std::string conteudo = "[PROPOSTA DE TAG]: " + tag;

    comando("./freechains chain '#regras' post --sign=" + pvt + " inline '" + conteudo + "' > /dev/null 2>&1");
    std::cout << "\nProposta de TAG publicada com sucesso\n\n";
    return;
    
}

// Funçao para publicar propostas de Subtags

void proporSubtag(const std::string& pub, const std::string& pvt) {
    
    system("clear");
    std::cout << "Carregando...\n";
    TagsemVigor();
    system("clear");
    
    std::cout << "Tags e Subtags existentes:\n";
    
    for (const auto& [tag, subtags] : tags) {
        std::cout << " - " << tag << ": ";
        for (size_t i = 0; i < subtags.size(); ++i) {
            std::cout << subtags[i];
            if (i != subtags.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
    }
    
    std::string tag;
    std::cout << "\n\nInforme a TAG que será relacionada á SUBTAG: \n ";
    std::getline(std::cin, tag);

    if (tags.find(tag) == tags.end()) {
        system("clear");
        std::cout << "ERRO: Insira uma TAG válida. Sua proposta foi recusada.\n";
        return;
    }
    
    std::string subtag;
    std::cout << "\n\nInforme a nova SUBTAG: \n ";
    std::getline(std::cin, subtag);
    const std::vector<std::string>& subtagsP = tags[tag];
    if ( (std::find(subtagsP.begin(), subtagsP.end(), subtag) != subtagsP.end()) || subtag.empty() || std::all_of(subtag.begin(), subtag.end(), isspace)){
        system("clear");
        std::cout << "ERRO: SUBTAG inválida. Sua proposta foi recusada.\n";
        return;
    }
    
    
    std::string conteudo = "[PROPOSTA DE SUBTAG]: TAG: " + tag + " SUBTAG: " + subtag;

    comando("./freechains chain '#regras' post --sign=" + pvt + " inline '" + conteudo + "' > /dev/null 2>&1");
    std::cout << "\nProposta de SUBTAG publicada com sucesso\n\n";
    return;
    
}

int main() {
    
    // host start
    
    system("./freechains-host start /tmp/no1 --no-tui > /dev/null 2>&1 &");

    std::string chave;
    int opcao;
    int reputacao = 0;
    std::cout << " -------  Aplicação Descentralizada de Regras  ------- \n\n";

    std::cout << "Informe sua chave única: ";
    std::getline(std::cin, chave);
    
    // gerando keys
    std::string chaves1 = comando("./freechains keys pubpvt \"" + chave + "\"");

    std::istringstream iss(chaves1);
    std::string pub, pvt;
    iss >> pub >> pvt;

    if (pub.empty() || pvt.empty()) {
        std::cerr << "Erro ao gerar as chaves. Verifique se o comando ./freechains está funcionando.\n";
        return 1;
    }
    
    // entrando na chain 
    
    std::string join = "./freechains chains join '#regras' " + pub + " > /dev/null 2>&1";
    system((join).c_str());

    system("clear");
    std::cout << "Carregando...\n\n";
    TagsemVigor();
    regrasVigor();
    system("clear");
    
    // view principal da aplicação

    while (true) {
        std::cout << " -------  Aplicação Descentralizada de Regras  ------- \n\n";

        std::cout << "[1] - Acessar Quadro de Regras \n";
        std::cout << "[2] - Propor Nova Regra \n";
        std::cout << "[3] - Propor Alteração de Regra \n";
        std::cout << "[4] - Propor Nova Tag \n";
        std::cout << "[5] - Propor Nova Subtag \n";
        std::cout << "[6] - Visualizar todas as propostas (consensus e bloqueadas) \n";
        std::cout << "[7] - Visualizar as propostas do consensus \n";
        std::cout << "[8] - Visualizar as propostas bloqueadas \n";
        std::cout << "[9] - Ver sua reputação \n";
        std::cout << "[0] - Sair\n\n";
        std::cout << "Digite uma opção: ";
        std::string entrada;
        std::getline(std::cin, entrada);
        try {
            opcao = std::stoi(entrada);
            } 
        catch (...) {
                opcao = 999;
            }

        switch (opcao) {
            case 1:
              verRegras(pub, pvt);
              break;
            case 2:
              proporRegra(pub, pvt);
              break;
            case 3:
              proporAlterar(pub, pvt);
              break;
            case 4:
              proporTag(pub, pvt);
              break;
            case 5:
              proporSubtag(pub, pvt);
              break;
            case 6:
              verPropostas(pub, pvt);
              break;
            case 7:
              verPropostasCons(pub, pvt);
              break;
            case 8:
              verPropostasBlocked(pub, pvt);
              break;
            case 9: 
              verRep(pub);
              break;
            case 0:
                std::cout << "Encerrando programa\n";
                return 0;
            default:
                std::cout << "Opção inválida\n";
                break;
        }
    }
    return 0;
}
